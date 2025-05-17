#pragma once

#include <WinSock2.h>
#include <Windows.h>

enum OperationType
{
	Accept,
	Recv,
	Send,
	NpcMove
};

class GameSession;
class RecvBuffer;
class IocpObject;

class ExpOver : public OVERLAPPED
{
public:
	ExpOver(OperationType operationType);

	void Init();

public:
	OperationType				_operationType;
	std::shared_ptr<IocpObject>	_owner{ nullptr };
};

class AcceptOver : public ExpOver
{
public:
	AcceptOver() : ExpOver(Accept) {}

public:
	char	_buffer[128]{ };	
	std::shared_ptr<GameSession> _session;
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

class NpcOver : public ExpOver
{
public:
	NpcOver(int npcId) : ExpOver(NpcMove), _npcId(npcId) {}

public:
	int _npcId{ -1 };
};
