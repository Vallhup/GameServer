#pragma once

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	void Init(const char* IP, u_short port);
	void Update();
	void Release();

	void Send(const std::vector<char>& packet);

	bool IsConnected() const;

private:
	SOCKET clientSocket;
	bool isConnected;

	char recvBuffer[1024];
};

