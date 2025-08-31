#include "pch.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Camera.h"
#include "ServerTestScene.h"

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
		OutputDebugStringA("WSAStartup Failed");
		return;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		OutputDebugStringA("INVALID SOCKET");
		WSACleanup();
		return;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, IP, &serverAddr.sin_addr);

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		OutputDebugStringA(("connect failed: " + std::to_string(error) + "\n").c_str());
		closesocket(clientSocket);
		WSACleanup();
		return;
	}

	u_long nonBlocking{ 1 };
	ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

	isConnected = true;

	OutputDebugStringA("Initialize Success");
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

		else if (WSAECONNRESET == error or WSAENOTCONN == error) {
			Release();
			return;
		}

		else {
			return;
		}
	}

	else if (0 >= recvLen) {
		Release();
		return;
	}

	else {
		recvBuffer.insert(recvBuffer.end(), tempBuffer, tempBuffer + recvLen);

		while (true) {
			if (recvBuffer.size() < sizeof(uint16_t)) {
				break;
			}

			uint16_t packetSize;
			memcpy(&packetSize, recvBuffer.data(), sizeof(uint16_t));

			if (packetSize <= 0 or packetSize > 4096) {
				recvBuffer.clear();
				break;
			}

			if (recvBuffer.size() < sizeof(uint16_t) + packetSize) {
				break;
			}

			std::vector<char> packet(recvBuffer.begin() + sizeof(uint16_t), recvBuffer.begin() + sizeof(uint16_t) + packetSize);
			recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + sizeof(uint16_t) + packetSize);

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
	}
	OutputDebugStringA("Release\n");
	WSACleanup();
}

void NetworkManager::Send(const std::vector<char>& packet)
{
	if (SceneManager* sManager = GET(Engine).GetSceneManager()) {
		if (Scene* scene = sManager->GetCurrentScene()) {
			if (auto testScene = dynamic_cast<ServerTestScene*>(scene)) {
				if (not isConnected or clientSocket == INVALID_SOCKET) {
					return;
				}

				int sent = send(clientSocket, packet.data(), packet.size(), 0);
				if (SOCKET_ERROR == sent) {
					int error = WSAGetLastError();
					std::string msg = "send() failed, error=" + std::to_string(error) + "\n";
					OutputDebugStringA(msg.c_str());

					if (WSAEWOULDBLOCK != error) {
						Release();
					}

					else {
						OutputDebugStringA(to_string(packet.size()).c_str());
					}
				}
			}
		}
	}
}

void NetworkManager::ProcessPacket(const std::vector<char>& packet)
{
	Protocol::GamePacket gamePacket;
	if (not gamePacket.ParseFromArray(packet.data(), packet.size())) {
		OutputDebugStringA("GamePacket Parsing failed");
		return;
	}

	// TEMP : Server Test
	if (SceneManager* sManager = GET(Engine).GetSceneManager()) {
		if (Scene* scene = sManager->GetCurrentScene()) {
			if (auto testScene = dynamic_cast<ServerTestScene*>(scene)) {
				testScene->HandlePacket(gamePacket);
			}
		}
	}

	/*const auto& header = gamePacket.header();
	switch (header.type()) {
	case Protocol::PacketType::SC_ADD: {

	}
	case Protocol::PacketType::SC_MOVE_OBJECT: {

	}
	case Protocol::PacketType::SC_REMOVE: {

	}
	default:
		break;
	}*/
}
