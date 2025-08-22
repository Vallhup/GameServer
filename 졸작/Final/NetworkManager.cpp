#include "pch.h"
#include "NetworkManager.h"

NetworkManager::~NetworkManager()
{
	if (isConnected) {
		Release();
	}
}

void NetworkManager::Initialize(const char* IP, u_short port)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup failed!" << endl;
		return;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		cout << "Socket creation failed!" << endl;
		WSACleanup();
		return;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, IP, &serverAddr.sin_addr);

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(clientSocket);
		WSACleanup();
		return;
	}

	u_long nonBlocking{ 1 };
	ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

	isConnected = true;
}

void NetworkManager::Update()
{
	if (not isConnected) {
		return;
	}

	char tempBuffer[1024];
	int recvLen = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
	if (SOCKET_ERROR == recvLen) {
		int error = WSAGetLastError();
		if (WSAEWOULDBLOCK == error) {
			return;
		}

		else {
			std::cerr << "recv() error : " << error << std::endl;
			Release();
			return;
		}
	}

	else if (0 >= recvLen) {
		std::cout << "Server DisConneted\n";
		Release();
		return;
	}

	else {
		recvBuffer.insert(recvBuffer.end(), tempBuffer, tempBuffer + recvLen);

		while (true) {
			if (recvBuffer.size() < sizeof(unsigned char)) {
				break;
			}

			unsigned char packetSize = static_cast<unsigned char>(recvBuffer[0]);

			if (packetSize <= 0 or packetSize > 32) {
				std::cerr << "Invalid Packet Size : " << (int)packetSize << std::endl;
				recvBuffer.clear();
				break;
			}

			if (recvBuffer.size() < packetSize) {
				break;
			}

			std::vector<char> packet(recvBuffer.begin(), recvBuffer.begin() + packetSize);
			recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + packetSize);

			ProcessPacket(packet);
		}
	}
}

void NetworkManager::Release()
{
	if (isConnected)
	{
		closesocket(clientSocket);
		isConnected = false;

		cout << "Socket closed!" << endl;
	}
	WSACleanup();
}

void NetworkManager::Send(const std::vector<char>& packet)
{
	if (SOCKET_ERROR == send(clientSocket, packet.data(), packet.size(), 0)) {
		int error = WSAGetLastError();
		if (WSAEWOULDBLOCK == error) {
			// 송신 버퍼 OverFlow -> 별도 처리 X (패킷 누락)
		}
		else {
			Release();
		}
	}
}

void NetworkManager::ProcessPacket(const std::vector<char>& packet)
{
	if (packet.size() < 2) return;

	const unsigned char packetType = packet[1];
	switch (packetType) {
		// TODO : Packet 처리

	default:
		std::cout << "[UNKNOWN PACKET] Type: " << (int)packetType << std::endl;
		break;
	}
}
