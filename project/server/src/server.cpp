#include <iostream>
#include <chrono>
#include "format.h"

#include "server.h"


Server* Server::RunningInstance = nullptr;

/////////////////////////////////////////////////////////////
///					PUBLIC FUNCTIONS				      ///
/////////////////////////////////////////////////////////////

Server::Server()
	: nPort(27020) {}

Server::Server(uint16 ServerPort)
	: nPort(ServerPort) {}

Server::~Server()
{
	if (m_NetworkThread.joinable())
		m_NetworkThread.join();
}

void Server::Start() {
	if (!m_Running) {
		m_Running = true;
		m_NetworkThread = std::thread([this]() { NetworkThreadFunction(); });
	}
}

void Server::Stop() {
	m_Running = false;
}

bool Server::isRunning() {
	return m_Running;
}

EResult Server::SendBufferToClient(const ClientInfo& Client, const Buffer& buffer, bool reliable) {
	return m_pInterface->SendMessageToConnection(Client.Connection, buffer.Data, (uint32)buffer.DataSize, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
}

void Server::SendBufferToAllClients(const Buffer& buffer, const ClientInfo& ExcludeClient, bool reliable) {
	std::map<HSteamNetConnection, ClientInfo> ConnectedClients;

	{
		std::lock_guard<std::mutex> lock(m_ConnectedClientsMutex);
		ConnectedClients = m_ConnectedClients;
	}

	for (const auto& [Connection, Client] : ConnectedClients) {
		if (ExcludeClient.Connection != k_HSteamNetConnection_Invalid && Connection == ExcludeClient.Connection)
			continue;

		SendBufferToClient(Client, buffer, reliable);
	}
}

void Server::SetDataReceivedCallback(const DataReceivedCallback& NewCallbackFunction) { m_DataReceivedCallback = NewCallbackFunction; }
void Server::SetClientConnectedCallback(const ClientConnectedCallback& NewCallbackFunction) { m_ClientConnectedCallback = NewCallbackFunction; } 
void Server::SetClientDisconnectedCallback(const ClientDisconnectedCallback& NewCallbackFunction) { m_ClientDisconnectedCallback = NewCallbackFunction; }

std::map<HSteamNetConnection, Server::ClientInfo> Server::GetConnectedClients() {
	std::lock_guard<std::mutex> lock(m_ConnectedClientsMutex);
	return Server::m_ConnectedClients;
}


/////////////////////////////////////////////////////////////
///					 PRIVATE FUNCTIONS				      ///
/////////////////////////////////////////////////////////////

void Server::NetworkThreadFunction() {
	RunningInstance = this;
	m_Running = true;

	SteamDatagramErrMsg errMsg;
	if (!GameNetworkingSockets_Init(nullptr, errMsg))
	{
		FatalError(fmt::format("GameNetworkingSockets_Init failed: {}", errMsg));
		return;
	}

	m_pInterface = SteamNetworkingSockets();

	SteamNetworkingIPAddr serverLocalAddr;
	serverLocalAddr.Clear();
	serverLocalAddr.m_port = nPort;

	SteamNetworkingConfigValue_t options;
	options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ConnectionStatusChangedCallback);

	m_ListenSocket = m_pInterface->CreateListenSocketIP(serverLocalAddr, 1, &options);
	char str[SteamNetworkingIPAddr::k_cchMaxString];
	serverLocalAddr.ToString(str, SteamNetworkingIPAddr::k_cchMaxString, true);
	std::cout << str << std::endl;
	if (m_ListenSocket == k_HSteamListenSocket_Invalid) {
		//fmt::format("Fatal error: Failed to listen on port {}", m_Port)
		FatalError(fmt::format("Fatal error: Failed to listen on port {}", nPort));
		return;
	}

	m_PollGroup = m_pInterface->CreatePollGroup();
	if (m_PollGroup == k_HSteamNetPollGroup_Invalid)
	{
		FatalError(fmt::format("Fatal error: Failed to listen on port {}", nPort));
		return;
	}

	std::cout << "Server Listening on port " << nPort << std::endl;

	while (m_Running) {
		PollIncomingMessages();
		PollConnectionStateChanges();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	std::cout << "Closing connections..." << std::endl;
	auto ConnectedClients = GetConnectedClients();

	for (const auto& [Connection, Client] : ConnectedClients)
	{
		m_pInterface->CloseConnection(Connection, 0, "Server Shutdown", true);
	}

	{
		std::lock_guard<std::mutex> lock(m_ConnectedClientsMutex);
		m_ConnectedClients.clear();
	}

	m_pInterface->CloseListenSocket(m_ListenSocket);
	m_ListenSocket = k_HSteamListenSocket_Invalid;

	m_pInterface->DestroyPollGroup(m_PollGroup);
	m_PollGroup = k_HSteamNetPollGroup_Invalid;
}

void Server::ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {
	RunningInstance->OnConnectionStatusChanged(pInfo);
}

void Server::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	// Handle connection state
	switch (pInfo->m_info.m_eState)
	{

	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections. You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
		{
			ClientInfo DisconnectedClient;
			bool HasDisconnectedClient = false;

			{
				std::lock_guard<std::mutex> lock(m_ConnectedClientsMutex);
				auto itClient = m_ConnectedClients.find(pInfo->m_hConn);

				if (itClient != m_ConnectedClients.end()) {
					DisconnectedClient = itClient->second;
					HasDisconnectedClient = true;
					m_ConnectedClients.erase(itClient);
				}
			}

			if (HasDisconnectedClient && (bool)m_ClientDisconnectedCallback)
				m_ClientDisconnectedCallback(DisconnectedClient);
		}

		m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);

		break;
	}

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		// We will get a callback immediately after accepting the connection.
		// Since we are the server, we can ignore this, it's not news to us.
		if (m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK) {
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			std::cout << "Couldn't accept connection" << std::endl;
			break;
		}

		if (!m_pInterface->SetConnectionPollGroup(pInfo->m_hConn, m_PollGroup)) {
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			std::cout << "Failed to set poll group" << std::endl;
			break;
		}

		std::cout << "Accepted Connection: " << pInfo->m_hConn << std::endl;

		SteamNetConnectionInfo_t connectionInfo;
		m_pInterface->GetConnectionInfo(pInfo->m_hConn, &connectionInfo);

		ClientInfo NewClient;
		NewClient.Connection = pInfo->m_hConn;
		NewClient.ConnectionDescription = connectionInfo.m_szConnectionDescription;

		{
			std::lock_guard<std::mutex> lock(m_ConnectedClientsMutex);
			m_ConnectedClients[pInfo->m_hConn] = NewClient;
		}

		if ((bool)m_ClientConnectedCallback)
			m_ClientConnectedCallback(NewClient);

		break;
	}

	case k_ESteamNetworkingConnectionState_Connected:
		// We will get a callback immediately after accepting the connection.
		// Since we are the server, we can ignore this, it's not news to us.
		break;

	default:
		break;
	}
}

void Server::PollIncomingMessages() {
	while (m_Running) {
		ISteamNetworkingMessage* pIncomingMessage = nullptr;
		int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup(m_PollGroup, &pIncomingMessage, 1);
		if (numMsgs == 0)
			break;

		if (numMsgs < 0)
		{
			FatalError("Error checking for messages");
			return;
		}

		ClientInfo MessageClient;

		{
			std::lock_guard<std::mutex> lock(m_ConnectedClientsMutex);
			auto itClient = m_ConnectedClients.find(pIncomingMessage->m_conn);

			if (itClient == m_ConnectedClients.end()) {
				std::cout << "Recived data from unregistered client" << std::endl;
				pIncomingMessage->Release();
				continue;
			}

			MessageClient = itClient->second;
		}

		if (pIncomingMessage->m_cbSize && (bool)m_DataReceivedCallback)
			m_DataReceivedCallback(MessageClient, Buffer(pIncomingMessage->m_pData, pIncomingMessage->m_cbSize));

		pIncomingMessage->Release();
	}
}

void Server::PollConnectionStateChanges()
{
	m_pInterface->RunCallbacks();
}

void Server::FatalError(const std::string& message) {
	std::cout << message << std::endl;
	m_Running = false;
}
