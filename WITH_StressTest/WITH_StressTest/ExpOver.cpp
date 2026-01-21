#include "pch.h"
#include "ExpOver.h"

ExpOver::ExpOver(OperationType opType) : _opType(opType)
{
	Init();
}

void ExpOver::Init()
{
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}

bool ExpOver::CheckOpType(OperationType opType)
{
	return _opType == opType;
}

RecvOver::RecvOver() : ExpOver(OperationType::Recv)
{
}

int RecvOver::SetBuffers()
{
	int totalFree = _buffer.GetFreeSize();
	int contiguousFree = _buffer.GetContiguousFreeSize();
	int firstRecvSize = std::min<int>(contiguousFree, totalFree);

	_wsaBuf[0].buf = reinterpret_cast<char*>(_buffer.GetWritePos());
	_wsaBuf[0].len = static_cast<ULONG>(firstRecvSize);

	int secondRecvSize = totalFree - firstRecvSize;
	if (secondRecvSize > 0) {
		_wsaBuf[1].buf = reinterpret_cast<char*>(_buffer.GetBuffer());
		_wsaBuf[1].len = static_cast<ULONG>(secondRecvSize);

		return 2;
	}

	return 1;
}

SendOver::SendOver() : ExpOver(OperationType::Send)
{
}

void SendOver::SetBuffers(std::vector<SendBuffer*>&& bufs)
{
	_buffers = std::move(bufs);
	_wsaBufs.clear();
	_wsaBufs.reserve(_buffers.size());

	for (auto* b : _buffers) 
	{
		WSABUF w;
		w.buf = reinterpret_cast<CHAR*>(b->data);
		w.len = b->size;
		_wsaBufs.push_back(w);
	}
}
