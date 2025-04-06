#pragma once

class Session;
class Packet;

// OVERLAPPED Struct 확장버전
// 어떤 session과 packet을 연결해주는 역할
class ExpOver
{
public:
	// Connect Recv 생성자
	ExpOver();

	// Recv 생성자
	ExpOver(Session* session);
	
	// Send 생성자
	ExpOver(Session* session, Packet* packet);

public:
	LPWSAOVERLAPPED	GetOverPtr() { return &_over; }
	LPWSABUF		GetWsabuf() { return _wsaBuf; }
	char*			GetBuffer() { return _buffer; }
	Session*		GetSession() const { return _session; }

private:
	WSAOVERLAPPED	_over;
	Session*		_session;
	char			_buffer[1024];
	WSABUF			_wsaBuf[1];
};