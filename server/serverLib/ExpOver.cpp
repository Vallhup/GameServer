#include "pch.h"

ExpOver::ExpOver()
{
	memset(&_over, 0, sizeof(_over));

	_session = nullptr;

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}

ExpOver::ExpOver(Session* session) : _session(session)
{
	memset(&_over, 0, sizeof(_over));

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}

ExpOver::ExpOver(Session* session, Packet* packet) : _session(session)
{
	memset(&_over, 0, sizeof(_over));

	std::memcpy(_buffer, packet, packet->GetSize());

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}
