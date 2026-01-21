#pragma once

#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

#include "IocpCore.h"
#include "ExpOver.h"
#include "vec3.h"
#include "Protocol.h"

enum class ClientState : char {
	ST_ALLOC,
	ST_INGAME,
	ST_FREE
};

struct SendBuffer;

class Client : public IocpObject {
	static constexpr int MAX_PACKET{ 32 };

public:
	Client() 
		: _id(-1), _state(ClientState::ST_ALLOC), _isSending(false), _socket(INVALID_SOCKET)
	{ _pos = { 0.0f, 0.0f, 0.0f }; _targetPos = { 0.0f, 0.0f, 0.0f }; }
	virtual ~Client() = default;

public:
	bool Connect();
	void Disconnect();

	void RegisterSend(SendBuffer* data);

	void Update(float deltaTime);

	int GetId() const { return _id; }
	const vec3& GetPos() const { return _pos; }
	ClientState GetState() const { return _state.load(); }
	bool IsConnected() const { return _state.load() != ClientState::ST_FREE; }

public:
	virtual HANDLE GetHandle() const override;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) override;

private:
	void RegisterRecv();

	void ProcessRecv(DWORD numBytes);
	void ProcessSend();

	void InternalSend();
	void ProcessPacket(const PacketHeader& header, const BYTE* data);

private:
	int _id;
	vec3 _pos;
	vec3 _targetPos;
	SOCKET _socket;
	RecvOver _recvOver;

	std::atomic<ClientState> _state;
	std::atomic<bool> _isSending;

	concurrency::concurrent_queue<SendBuffer*> _sendQueue;
};