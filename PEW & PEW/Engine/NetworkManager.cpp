#include "pch.h"
#include "NetworkManager.h"

NetworkManager::NetworkManager()
{
	clientSocket = INVALID_SOCKET;
	isConnected = false;
}

NetworkManager::~NetworkManager()
{
	if (isConnected)
		Release();
}

void NetworkManager::Init(const char* IP, u_short port) 
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
		cout << "Connection failed!" << endl;
		closesocket(clientSocket);
		WSACleanup();
		return;
	}

	// Socket을 Non-Blocking으로 변경
	u_long nonBlocking{ 1 };
	ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

	isConnected = true;
	cout << "Connected to Server!" << endl;
}

void NetworkManager::Update() 
{
	// 1. read set 초기화, socket Setting
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(clientSocket, &readSet);

	// 2. timeout 설정, Select
	timeval timeout{ 0, 0 }; // Non-Blocking Select를 위해 timeout 0으로 설정
	if (select(0, &readSet, nullptr, nullptr, &timeout) <= 0) {
		return;
	}

	// 3. Recv 가능한지 판별, 가능하면 Recv 진행
	if (FD_ISSET(clientSocket, &readSet)) {
		int recvLen = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (recvLen <= 0) {
			Release();
			return;
		}

		// Packet 분리 작업
		int offset{ 0 };
		while (offset < recvLen) {
			if (offset + sizeof(int) > recvLen) break;
			int size = *(reinterpret_cast<int*>(recvBuffer + offset));
			if (size <= 0 or offset + size > recvLen) break;

			std::vector<char> packet(recvBuffer + offset, recvBuffer + offset + size);
			
			std::cout << packet.data() << std::endl;
			// TODO : Packet 처리
			// 간단하게 만들면 NetworkManager에서 구현할 수도 있음 (근데 마음에 안듦)
			// 제대로 할거면 따로 ObjecetManager, GameManager 같은 곳에서 Packet 처리를 구현해서 추가해야됨

			offset += size;
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

bool NetworkManager::IsConnected() const
{
	return isConnected;
}