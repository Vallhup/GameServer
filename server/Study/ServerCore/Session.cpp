#include "pch.h"
#include "Session.h"

bool Session::ProcessPacket(char* packet)
{
	char packetType = packet[1];

	switch (packetType)
	{
	case PACKET_CONNECT: {
		ResponseConnectPacket responsePacket(_id);
		doSend(&responsePacket);

		break;
	}
	case PACKET_DISCONNECT:
		break;

	case PACKET_MOVE: {
		RequestMovePacket* requestPacket = reinterpret_cast<RequestMovePacket*>(packet);
		switch (requestPacket->_direction) {
		case MOVE_UP:    _pos._yPos = std::min<short>(_pos._yPos + 1, 8); break;
		case MOVE_DOWN:  _pos._yPos = std::max<short>(_pos._yPos - 1, 0); break;
		case MOVE_LEFT:  _pos._xPos = std::min<short>(_pos._xPos + 1, 8); break;
		case MOVE_RIGHT: _pos._xPos = std::max<short>(_pos._xPos - 1, 0); break;
		}

		ResponseMovePacket responsePacket(_id, _pos);
		doSend(&responsePacket);

		break;
	}
	}

	return true;
}

void Session::doRecv()
{
	DWORD recvFlag = 0;

	_recvOver.Init();
	_recvOver._owner = shared_from_this();
	
	_recvOver._wsaBuf[0].buf = _recvOver._buffer.GetBuffer() + _recvOver._buffer.GetWritePos();
	_recvOver._wsaBuf[0].len = _recvOver._buffer.GetFreeSize();

	int result = WSARecv(_socket, _recvOver._wsaBuf, 1, NULL, &recvFlag, reinterpret_cast<LPWSAOVERLAPPED>(&_recvOver), gRecvCallback);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			errorDisplay("Recv : ", error);
		}
	}
}

void Session::doSend(void* packet)
{
	DWORD sizeSent;

	_sendOver.Init();
	_sendOver._owner = shared_from_this();

	const unsigned char packetSize = reinterpret_cast<unsigned char*>(packet)[0];
	std::memcpy(_sendOver._buffer, packet, packetSize);

	_sendOver._wsaBuf[0].buf = _sendOver._buffer;
	_sendOver._wsaBuf[0].len = packetSize;

	WSASend(_socket, _sendOver._wsaBuf, 1, &sizeSent, 0, reinterpret_cast<LPWSAOVERLAPPED>(&_sendOver), gSendCallback);
}

void Session::RecvCallback(DWORD numBytes)
{
	_recvOver._owner = nullptr;
	_recvOver._buffer.Write(nullptr, numBytes);

	std::vector<char> readBuffer(numBytes);
	_recvOver._buffer.Read(readBuffer.data(), numBytes);

	ProcessPacket(readBuffer.data());
	
	doRecv();
}

void Session::SendCallback()
{
	_sendOver._owner = nullptr;
}
