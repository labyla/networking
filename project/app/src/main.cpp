#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "client.h"
#include "server.h"

namespace {

constexpr uint16 kDefaultServerPort = 27020;
constexpr auto kPacketSendInterval = std::chrono::seconds(3);

void PrintUsage(const char* ExecutableName) {
	std::cout << "Usage:" << std::endl;
	std::cout << "  " << ExecutableName << " server [port]" << std::endl;
	std::cout << "  " << ExecutableName << " client <address:port>" << std::endl;
}

bool ParsePort(const char* PortArgument, uint16& OutPort) {
	char* End = nullptr;
	errno = 0;
	unsigned long ParsedPort = std::strtoul(PortArgument, &End, 10);

	if (errno != 0 || End == PortArgument || *End != '\0' || ParsedPort == 0 || ParsedPort > 65535)
		return false;

	OutPort = static_cast<uint16>(ParsedPort);
	return true;
}

void WaitForStopCommand() {
	std::cout << "Press Enter to stop..." << std::endl;

	std::string Line;
	std::getline(std::cin, Line);
}

void WaitForNextPacket(const std::atomic<bool>& Running) {
	auto WaitedTime = std::chrono::milliseconds(0);

	while (Running && WaitedTime < kPacketSendInterval) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		WaitedTime += std::chrono::milliseconds(100);
	}
}

int RunServer(int argc, char** argv) {
	uint16 ServerPort = kDefaultServerPort;

	if (argc > 3) {
		PrintUsage(argv[0]);
		return 1;
	}

	if (argc == 3 && !ParsePort(argv[2], ServerPort)) {
		std::cout << "Invalid server port: " << argv[2] << std::endl;
		return 1;
	}

	Server ServerInstance(ServerPort);

	ServerInstance.SetClientConnectedCallback([](const Server::ClientInfo& Client) {
		std::cout << "Client connected: " << Client.ConnectionDescription << std::endl;
	});

	ServerInstance.SetClientDisconnectedCallback([](const Server::ClientInfo& Client) {
		std::cout << "Client disconnected: " << Client.ConnectionDescription << std::endl;
	});

	ServerInstance.SetDataReceivedCallback([&ServerInstance](const Server::ClientInfo& Client, const Buffer& ReceivedBuffer) {
		std::cout << "Received " << ReceivedBuffer.DataSize << " bytes from " << Client.ConnectionDescription << std::endl;
		ServerInstance.SendBufferToClient(Client, ReceivedBuffer);
	});

	ServerInstance.Start();
	std::cout << "Starting server on port " << ServerPort << std::endl;

	std::atomic<bool> SendPackets = true;
	std::thread PacketThread([&ServerInstance, &SendPackets]() {
		uint32 PacketIndex = 0;
		Server::ClientInfo ExcludeClient{ k_HSteamNetConnection_Invalid, "" };

		while (SendPackets) {
			WaitForNextPacket(SendPackets);

			if (!SendPackets || !ServerInstance.isRunning())
				break;

			auto ConnectedClients = ServerInstance.GetConnectedClients();
			if (ConnectedClients.empty())
				continue;

			std::string Packet = "Server packet " + std::to_string(++PacketIndex);
			ServerInstance.SendBufferToAllClients(Buffer(Packet.data(), (int)Packet.size()), ExcludeClient);
			std::cout << "Sent packet to " << ConnectedClients.size() << " clients: " << Packet << std::endl;
		}
	});

	WaitForStopCommand();
	SendPackets = false;

	if (PacketThread.joinable())
		PacketThread.join();

	ServerInstance.Stop();

	return 0;
}

int RunClient(int argc, char** argv) {
	if (argc != 3) {
		PrintUsage(argv[0]);
		return 1;
	}

	std::string ServerAddress = argv[2];
	Client ClientInstance;

	ClientInstance.SetServerConnectedCallback([]() {
		std::cout << "Connected to server" << std::endl;
	});

	ClientInstance.SetServerDisconnectedCallback([]() {
		std::cout << "Disconnected from server" << std::endl;
	});

	ClientInstance.SetDataReceivedCallback([](const Buffer& ReceivedBuffer) {
		std::cout << "Received " << ReceivedBuffer.DataSize << " bytes from server" << std::endl;
	});

	ClientInstance.ConnectToServer(ServerAddress);
	std::cout << "Connecting to " << ServerAddress << std::endl;

	std::atomic<bool> SendPackets = true;
	std::thread PacketThread([&ClientInstance, &SendPackets]() {
		uint32 PacketIndex = 0;

		while (SendPackets) {
			WaitForNextPacket(SendPackets);

			if (!SendPackets || !ClientInstance.isRunning())
				break;

			if (ClientInstance.GetConnectionStatus() != Client::ConnectionStatus::Connected)
				continue;

			std::string Packet = "Client packet " + std::to_string(++PacketIndex);
			ClientInstance.SendBufferToServer(Buffer(Packet.data(), (int)Packet.size()));
			std::cout << "Sent packet to server: " << Packet << std::endl;
		}
	});

	WaitForStopCommand();
	SendPackets = false;

	if (PacketThread.joinable())
		PacketThread.join();

	ClientInstance.Disconnect();

	return 0;
}

} // namespace

int main(int argc, char** argv) {
	if (argc < 2) {
		PrintUsage(argv[0]);
		return 1;
	}

	if (std::strcmp(argv[1], "server") == 0)
		return RunServer(argc, argv);

	if (std::strcmp(argv[1], "client") == 0)
		return RunClient(argc, argv);

	std::cout << "Unknown application mode: " << argv[1] << std::endl;
	PrintUsage(argv[0]);
	return 1;
}
