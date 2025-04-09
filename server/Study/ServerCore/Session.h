#pragma once

#include "ExpOver.h"

struct Pos
{
	short _xPos;
	short _yPos;
};

// Client의 정보 (고유 id, 연결 socket 등)
// 컨텐츠와 관련된 작업들
class Session : public std::enable_shared_from_this<Session>
{
public:
	Session() : _id(0), _socket(INVALID_SOCKET), _pos(0, 0) {}
	Session(int id, SOCKET s) : _id(id), _socket(s), _pos(0, 0)
	{
		doRecv();
	}

	~Session() { closesocket(_socket); }

public:
	bool ProcessPacket(char* packet);

public:
	// Getter
	SOCKET GetSocket() const { return _socket; }
	int    GetSessionId() const { return _id; }
	Pos	   GetPos() const { return _pos; }

public:
	void doRecv();
	void doSend(void* packet);

	void RecvCallback(DWORD numBytes);
	void SendCallback();

private:
	SOCKET	_socket;
	int		_id;
	Pos		_pos;

private:
	AcceptOver	_acceptOver;
	RecvOver	_recvOver;
	SendOver	_sendOver;
};
