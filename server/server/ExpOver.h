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

	// Recv 생성자
	ExpOver(Session* session) : _session(session)
	{
		memset(&_over, 0, sizeof(_over));

		_wsaBuf[0].buf = _buffer;
		_wsaBuf[0].len = sizeof(_buffer);
	}

	// Send 생성자
	ExpOver(Session* session, const Packet& packet) : _session(session)
	{
		memset(&_over, 0, sizeof(_over));
		
		// 왜 packet의 Getter를 안쓰고?
		// packet이 저장하는 size는 하위 클래스에 들어 있는 packet body size이기 때문
		// 지금은 packet 전체를 복사해야 됨
		std::memcpy(_buffer, &packet, sizeof(packet));

		_wsaBuf[0].buf = _buffer;
		_wsaBuf[0].len = static_cast<ULONG>(packet.GetSize());
	}

private:
	WSAOVERLAPPED	_over;
	Session*		_session;
	char			_buffer[1024];
	WSABUF			_wsaBuf[1];
};