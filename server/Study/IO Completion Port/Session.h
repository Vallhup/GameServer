#pragma once

#include "ExpOver.h"
#include "IocpCore.h"

struct Pos
{
	short _xPos;
	short _yPos;
};


// Client의 정보 (고유 id, 연결 socket 등)
// 컨텐츠와 관련된 작업들
class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

public:
	Session()
	{
		_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	}

	virtual ~Session();

public:
	void Send(const std::vector<char>& data);
	virtual bool ProcessPacket(const std::vector<char>& packet) abstract;

public:
	// Getter
	SOCKET GetSocket() const { return _socket; }
	int    GetSessionId() const { return _id; }

	// Setter
	void SetService(std::shared_ptr<Service> service) { _service = service; }
	void SetId(int id) { _id = id; };

public:
	void doRecv();
	void doSend();

	void RecvCallback(DWORD numBytes);
	void SendCallback();

private:
	// Interface 구현
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) override;

protected:
	std::weak_ptr<Service> _service;
	SOCKET	_socket;

protected:
	AtomicQueue<std::vector<char>> _sendQueue;
	std::atomic<bool> _isSending{ false };

protected:
	RecvOver	_recvOver;
	SendOver	_sendOver;
};

class GameSession : public Session {
	friend class Service;

public:
	virtual ~GameSession() override;

public:
	virtual bool ProcessPacket(const std::vector<char>& packet) override;

public:
	Pos	   GetPos() const { return _pos; }

private:
	Pos		_pos{ 4, 4 };
};