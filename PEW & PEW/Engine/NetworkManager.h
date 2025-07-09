#pragma once

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	void Init(const char* IP, u_short port);
	void Update();
	void Release();

	bool IsConnected() const;

private:
	SOCKET clientSocket;
	bool isConnected;
};

