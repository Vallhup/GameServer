#pragma once
#include "RecvBuffer.h"

class RemotePlayer;

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	void Init(const char* IP, u_short port);
	void Update();
	void Release();

	void Send(const std::vector<char>& packet);
	void ProcessPacket(const std::vector<char>& packet);

	bool IsConnected() const;

	const std::map<int, RemotePlayer*>& GetRemotePlayers() const { return remotePlayers; }

private:
	SOCKET clientSocket;
	bool isConnected;

	RecvBuffer recvBuffer;
	std::map<int, RemotePlayer*> remotePlayers;
};

