#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include "RecvBuffer.h"
#include "IocpCore.h"

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
	std::shared_ptr<IocpObject>	_owner;
};

class AcceptOver : public ExpOver
{
public:
	AcceptOver() : ExpOver(Accept) {}

public:
	char	_buffer[128]{ };	
	std::shared_ptr<Session> _session;
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
	SendOver() : ExpOver(Send) {}

	void SetBuffer(const std::shared_ptr<std::vector<char>>& data)
	{
		_sendData = data;

		_wsaBuf[0].buf = const_cast<char*>(data->data());
		_wsaBuf[0].len = static_cast<ULONG>(data->size());
	}

public:
	WSABUF		_wsaBuf[1];
	std::shared_ptr<std::vector<char>> _sendData;
};