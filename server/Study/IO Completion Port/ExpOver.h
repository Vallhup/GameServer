#pragma once

#include <WinSock2.h>
#include <Windows.h>

enum OperationType : char
{
	Accept,
	Recv,
	Send,
	Heal,
	NpcMove,
	NpcHeal,
	NpcAttack
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
	RecvOver() : ExpOver(Recv) {}

	int PrepareWSABufs()
	{
		int totalFree = _buffer.GetFreeSize();
		int contiguousFree = _buffer.GetContiguousFreeSize();
		int firstRecvSize = std::min<int>(contiguousFree, totalFree);

		_wsaBuf[0].buf = _buffer.GetWritePos();
		_wsaBuf[0].len = static_cast<ULONG>(firstRecvSize);

		int secondRecvSize = totalFree - firstRecvSize;
		if (secondRecvSize > 0) {
			_wsaBuf[1].buf = _buffer.GetBuffer();
			_wsaBuf[1].len = static_cast<ULONG>(secondRecvSize);

			return 2;
		}

		return 1;
	}

public:
	RecvBuffer	_buffer;
	WSABUF		_wsaBuf[2];
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

class EventOver : public ExpOver
{
public:
	EventOver(OperationType op, int id) : ExpOver(op), _id(id) {}

public:
	int _id{ -1 };
};
