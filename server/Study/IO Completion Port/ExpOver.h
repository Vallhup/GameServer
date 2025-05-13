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

	void SetBuffers(const std::vector<std::shared_ptr<std::vector<char>>>&& buffers)
	{
		_wsaBufs.clear();
		_sendDataList.clear();

		_sendDataList = buffers;
		_wsaBufs.resize(buffers.size());
		
		for (size_t i = 0; i < buffers.size(); ++i) {
			_wsaBufs[i].buf = const_cast<char*>(_sendDataList[i]->data());
			_wsaBufs[i].len = static_cast<ULONG>(_sendDataList[i]->size());
		}
	}

public:
	std::vector<WSABUF>		_wsaBufs;
	std::vector<std::shared_ptr<std::vector<char>>> _sendDataList;
};