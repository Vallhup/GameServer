#pragma once

#include "RecvBuffer.h"

class Service;

class Session
{
public:
	Session(int sessionId, SOCKET socket);
	~Session();

	bool Recv();
	bool Send(const std::vector<char>& data);
	
	void OnConnect(Service* service);
	void DisConnect();

	void ProcessPacket();
	void HandlePacket(const std::vector<char>& packet);

public:
	int GetId() const { return _id; }
	SOCKET GetSocket() const { return _socket; }
	std::shared_ptr<Character> GetCharacter() const { return _character; }
	
	void SetCharacter(const std::shared_ptr<Character>& character) { _character = character; }

private:
	void HandleMovePacket(const std::vector<char>& packet);
	void HandleAttackPacket(const std::vector<char>& packet);

private:
	int _id;

	std::shared_ptr<Character> _character;

	SOCKET _socket;
	RecvBuffer _recvBuffer;

	bool _isConnected{ false };
};

