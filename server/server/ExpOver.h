#pragma once

#include <WS2tcpip.h>

#pragma comment (lib, "WS2_32.LIB")

class ExpOver
{
public:
	ExpOver() = delete;
	ExpOver(long long id, char* message) : _id(id)
	{
		memset(&_over, 0, sizeof(_over));

		
	}


private:
	WSAOVERLAPPED	_over;
	long long		_id;
	char			_buffer[1024];
	WSABUF			_wsaBuf[1];
};