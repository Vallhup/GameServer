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

	isConnected = true;
	cout << "Connected to Server!" << endl;
}

void NetworkManager::Update() 
{

}

void NetworkManager::Release()
{
	if (isConnected)
	{
		closesocket(clientSocket);
		isConnected = false;
	}
	WSACleanup();

	cout << "Socket closed!" << endl;
}

bool NetworkManager::IsConnected() const
{
	return isConnected;
}