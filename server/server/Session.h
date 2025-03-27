#pragma once

#include <WS2tcpip.h>

#pragma comment (lib, "WS2_32.LIB")

class Session
{
public:
	Session() = delete;
	Session(long long id, SOCKET s) : _id(id), _socket(s)
	{
		_recvWsabuf[0].buf = _recvBuffer;
		_recvWsabuf[0].len = sizeof(_recvBuffer);


	}

private:
	SOCKET			_socket;
	long long		_id;

	WSAOVERLAPPED	_recvOver;
	char			_recvBuffer[1024];
	WSABUF			_recvWsabuf[1];

	int _test4;

};