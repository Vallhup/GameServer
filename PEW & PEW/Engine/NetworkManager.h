#pragma once
#include "RecvBuffer.h"

class GraphicsManager;

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

	void SetGraphicsManager(GraphicsManager* gfx) { graphics = gfx; }

private:
	SOCKET clientSocket;
	bool isConnected;

	RecvBuffer recvBuffer;
	GraphicsManager* graphics = nullptr;
};

