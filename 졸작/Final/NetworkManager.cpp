#include "pch.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Camera.h"
#include "GameScene.h"

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
			if (recvBuffer.size() < sizeof(PacketHeader)) break;

			PacketHeader header;
			PacketFactory::PeekHeader(recvBuffer.data(), recvBuffer.size(), &header);

			if (header.size <= 0 or header.size > 4096)
			{
				recvBuffer.clear();
				break;
			}

			const int totalSize = header.size;
			if (recvBuffer.size() < totalSize) break;

			std::vector<char> packet(recvBuffer.begin(),
				recvBuffer.begin() + totalSize);
			recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + totalSize);

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
			if (auto testScene = dynamic_cast<GameScene*>(scene)) {
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
	// Update 루프에서 패킷재조립 완료된 상태로 packet이 보내짐
	volatile int a = 1;
	std::cout << a << std::endl;

	const PacketHeader* header = reinterpret_cast<const PacketHeader*>(packet.data());
	//const char* body = packet.data() + sizeof(PacketHeader);

	// TEMP : Server Test
	if (SceneManager* sManager = GET(Engine).GetSceneManager())
	{
		if (Scene* scene = sManager->GetCurrentScene())
		{
			if (auto testScene = dynamic_cast<GameScene*>(scene))
				testScene->HandlePacket(header);
		}
	}
}