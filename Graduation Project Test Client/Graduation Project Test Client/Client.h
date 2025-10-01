#pragma once

#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

#include <concurrent_queue.h>
#include <atomic>

#include "IocpCore.h"
#include "ExpOver.h"
#include "vec3.h"

class Client : public IocpObject {
	static constexpr int MAX_PACKET{ 32 };

public:
	Client() 
		: _id(-1), _connected(false), _isSending(false), _socket(INVALID_SOCKET) 
	{ _pos = { 0.0f, 0.0f, 0.0f }; }
	virtual ~Client() = default;

public:
	bool Connect();
	void Disconnect();

	void RegisterSend(const std::vector<char>& packet);

	int GetId() const { return _id; }
	bool IsConnected() const { return _connected.load(); }

public:
	virtual HANDLE GetHandle() const override;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) override;

private:
	void RegisterRecv();

	void ProcessRecv(DWORD numBytes);
	void ProcessSend();

	void InternalSend();
	void ProcessPacket(const std::vector<char>& packet);

private:
	int _id;
	vec3 _pos;
	SOCKET _socket;
	RecvOver _recvOver;

	std::atomic<bool> _connected;
	std::atomic<bool> _isSending;

	concurrency::concurrent_queue<std::vector<char>> _sendQueue;
};