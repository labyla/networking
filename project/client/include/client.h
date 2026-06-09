#pragma once

#include <string>
#include <thread>
#include <functional>

#include <steamnetworkingsockets.h>
#include <isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam_api.h>
#endif

#include "buffer.h"

class Client {
public:
	enum ConnectionStatus {
		Disconnected = 0, Connected, Connecting, FailedToConnect
	};

	using DataReceivedCallback = std::function<void(const Buffer&)>;
	using ServerConnectedCallback = std::function<void()>;
	using ServerDisconnectedCallback = std::function<void()>;

	static Client* RunningInstance;

public:

	Client() = default;

	~Client();

	void ConnectToServer(const std::string& ServerAddress);
	
	void Disconnect();

	EResult SendBufferToServer(const Buffer& buffer, bool reliable = true);

	template <typename T>
	EResult SendDataToServer(const T& Data, bool reliable = true) {
		return SendBufferToServer(Buffer(&Data, sizeof(T)), reliable);
	}

	void SetDataReceivedCallback(const DataReceivedCallback& NewCallbackFunction);
	void SetServerConnectedCallback(const ServerConnectedCallback& NewCallbackFunction);
	void SetServerDisconnectedCallback(const ServerDisconnectedCallback& NewCallbackFunction);

	bool isRunning();

	ConnectionStatus GetConnectionStatus();
	const std::string& GetConnectionDebugMessage();

private:
	/////////////////////////////////////////////////////////////
	///						 VARIABLES						  ///
	/////////////////////////////////////////////////////////////

	std::thread m_NetworkThread;
	std::string m_ServerAddress;
	bool m_Running = false;

	HSteamNetConnection m_Connection;
	ConnectionStatus m_ConnectionStatus = ConnectionStatus::Disconnected;
	std::string m_ConnectionDebugMessage;
	ISteamNetworkingSockets* m_pInterface;

	DataReceivedCallback m_DataReceivedCallback;
	ServerConnectedCallback m_ServerConnectedCallback;
	ServerDisconnectedCallback m_ServerDisconnectedCallback;


	/////////////////////////////////////////////////////////////
	///						 FUNCTIONS						  ///
	/////////////////////////////////////////////////////////////

	void NetworkThreadFunction();

	void PollIncomingMessages();

	void PollConnectionStateChanges();

	static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	void FatalError(const std::string& message);
};
