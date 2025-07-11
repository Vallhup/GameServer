#pragma once

#include "RecvBuffer.h"

class Session
{
public:
	Session(int sessionId, SOCKET socket);
	~Session();

	int GetId() const { return _id; }
	SOCKET GetSocket() const { return _socket; }

	bool Recv();
	bool Send(const std::vector<char>& data);
	
	void OnConnect();
	void DisConnect();

	void ProcessPacket();
	void HandlePacket(const std::vector<char>& packet);

private:
	void HandleMovePacket(const std::vector<char>& packet);
	void HandleAttackPacket(const std::vector<char>& packet);

private:
	int _id;
	SOCKET _socket;

	RecvBuffer _recvBuffer;

	bool _isConnected{ false };
};

