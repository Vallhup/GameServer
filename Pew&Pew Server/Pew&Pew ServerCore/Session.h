#pragma once

#include "RecvBuffer.h"

class Service;

class Session {
	using PacketHandler = std::function<void(int, const std::vector<char>&)>;

public:
	Session(int sessionId, SOCKET socket);
	~Session();

	bool Recv();
	bool Send(const std::vector<char>& data);
	
	void OnConnect();
	void DisConnect();

	void ProcessPacket();
	//void HandlePacket(const std::vector<char>& packet);

public:
	int GetId() const { return _id; }
	SOCKET GetSocket() const { return _socket; }
	std::shared_ptr<Character> GetCharacter() const { return _character; }
	
	void SetCharacter(const std::shared_ptr<Character>& character) { _character = character; }
	void SetPacketHandler(PacketHandler handler) { _packetHandler = handler; }

private:
	bool InternalSend();

private:
	int _id;

	SOCKET _socket;
	RecvBuffer _recvBuffer;

	concurrency::concurrent_queue<std::vector<char>> _sendQueue;
	std::atomic<bool> _isSending{ false };

	std::shared_ptr<Character> _character{ nullptr };

	PacketHandler _packetHandler;

	bool _isConnected{ false };
};

