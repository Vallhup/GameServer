#include "pch.h"

ExpOver::ExpOver()
{
	memset(&_over, 0, sizeof(_over));
	//memset(&_buffer, 0, sizeof(_buffer));

	_session = nullptr;

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}

ExpOver::ExpOver(Session* session) : _session(session)
{
	memset(&_over, 0, sizeof(_over));
	//memset(&_buffer, 0, sizeof(_buffer));

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}

ExpOver::ExpOver(Session* session, Packet* packet) : _session(session)
{
	memset(&_over, 0, sizeof(_over));
	//memset(&_buffer, 0, sizeof(_buffer));

	std::memcpy(_buffer, packet, packet->GetSize());

	_wsaBuf[0].buf = _buffer;
	_wsaBuf[0].len = sizeof(_buffer);
}
