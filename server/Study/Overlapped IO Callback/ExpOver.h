#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include "RecvBuffer.h"

enum OperationType
{
	Accept,
	Recv,
	Send
};

class Session;

class ExpOver : public OVERLAPPED
{
public:
	ExpOver(OperationType operationType);

	void Init();

public:
	OperationType				_operationType;
	std::shared_ptr<Session>	_owner;
};

class AcceptOver : public ExpOver
{
public:
	AcceptOver() : ExpOver(Accept) {}

public:
	SOCKET	_clientSocket;
	char*	_buffer;
};

class RecvOver : public ExpOver
{
public:
	RecvOver() : ExpOver(Recv) 
	{
		_wsaBuf[0].buf = _buffer.GetBuffer();
		_wsaBuf[0].len = _buffer.GetFreeSize();
	}

public:
	RecvBuffer	_buffer;
	WSABUF		_wsaBuf[1];
};

class SendOver : public ExpOver
{
public:
	SendOver() : ExpOver(Send) 
	{
		_wsaBuf[0].buf = _buffer;
		_wsaBuf[0].len = sizeof(_buffer);
	}

public:
	char		_buffer[2048];
	WSABUF		_wsaBuf[1];
};