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
	bool CanStart() const;

	void SetGraphicsManager(GraphicsManager* gfx) { graphics = gfx; }

private:
	SOCKET clientSocket;
	bool isConnected;
	bool canStart = false;

	std::vector<char> recvBuffer;
	GraphicsManager* graphics = { nullptr };
};

