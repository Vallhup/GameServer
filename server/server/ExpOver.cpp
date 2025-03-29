#include "ExpOver.h"
#include "Packet.h"


ExpOver::ExpOver(Session* session) : _session(session)
{
	memset(&_over, 0, sizeof(_over));
	memset(&_buffer, 0, sizeof(_buffer));

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}

ExpOver::ExpOver(Session* session, const Packet& packet) : _session(session)
{
	memset(&_over, 0, sizeof(_over));

	// 왜 packet의 GetSize()를 안쓰고?
	// packet이 저장하는 size는 하위 클래스에 들어 있는 packet body size이기 때문
	// 지금은 packet 전체를 복사해야 됨
	std::memcpy(_buffer, &packet, sizeof(packet));

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = static_cast<ULONG>(packet.GetSize());
}
