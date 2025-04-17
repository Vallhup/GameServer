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
	Session() : _id(0), _socket(INVALID_SOCKET), _pos{ 4, 4 } {}
	Session(int id, SOCKET s) : _id(id), _socket(s), _pos{ 4, 4 } {}

	~Session();

public:
	bool ProcessPacket(char* packet);

public:
	// Getter
	SOCKET GetSocket() const { return _socket; }
	int    GetSessionId() const { return _id; }
	Pos	   GetPos() const { return _pos; }

	// Setter
	void SetService(std::shared_ptr<Service> service) { _service = service; }

public:
	void doRecv();
	void doSend(void* packet);

	void RecvCallback(DWORD numBytes);
	void SendCallback();

private:
	// Interface 구현
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) override;

private:
	std::weak_ptr<Service> _service;
	SOCKET	_socket;
	int		_id;
	Pos		_pos;

private:
	RecvOver	_recvOver;
	SendOver	_sendOver;
};
