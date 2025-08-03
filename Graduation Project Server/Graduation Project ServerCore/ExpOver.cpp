#include "pch.h"
#include "ExpOver.h"

ExpOver::ExpOver(OperationType opType) : _opType(opType)
{
	memset(this, 0, sizeof(this));
}

bool ExpOver::CheckOpType(OperationType opType)
{
	return _opType == opType;
}

AcceptOver::AcceptOver() : ExpOver(Accept)
{
	memset(_buffer, 0, sizeof(_buffer));
	_socket = INVALID_SOCKET;
}

RecvOver::RecvOver() : ExpOver(Recv)
{
}

int RecvOver::SetBuffers()
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

SendOver::SendOver() : ExpOver(Send)
{
}

void SendOver::SetBuffers(const std::vector<std::vector<char>>&& buffers)
{
	_wsaBufs.clear();
	_sendDataList.clear();

	_sendDataList = buffers;
	_wsaBufs.resize(buffers.size());

	for (size_t i = 0; i < buffers.size(); ++i) {
		_wsaBufs[i].buf = const_cast<char*>(_sendDataList[i].data());
		_wsaBufs[i].len = static_cast<ULONG>(_sendDataList[i].size());
	}
}
