#pragma once

#include <iostream>
#include <WS2tcpip.h>
#include "Packet.h"
#include "Session.h"

#pragma comment (lib, "WS2_32.LIB")

// OVERLAPPED Struct 확장버전
// 어떤 session과 packet을 연결해주는 역할
class ExpOver
{
public:
	ExpOver() = delete;
	ExpOver(const Packet& packet)
	{
		memset(&_over, 0, sizeof(_over));

		auto packetSize = 2 + packet.GetData().size();
		if (packetSize > 1024) {
			std::cout << "Packet Size Over";
			exit(-1);
		}

		_buffer[0] = static_cast<unsigned char>(packet.GetSize());
		_buffer[1] = static_cast<unsigned char>(packet.GetSessionId());

		std::copy(packet.GetData().begin(), packet.GetData().end(), _buffer + 2);

		_wsaBuf[0].buf = _buffer;
		_wsaBuf[0].len = static_cast<ULONG>(packet.GetSize());
	}

private:
	WSAOVERLAPPED	_over;
	Session*		_session;
	char			_buffer[1024];
	WSABUF			_wsaBuf[1];
};