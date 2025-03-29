#pragma once

#include <WS2tcpip.h>

#pragma comment (lib, "WS2_32.LIB")

// Client의 정보 (고유 id, 연결 socket 등)
// 컨텐츠와 관련된 작업들
class Session
{
public:
	Session() = delete;
	Session(int sessionId, SOCKET s) : _sessionId(sessionId), _socket(s), _xPos(0), _yPos(0)
	{
	}

	~Session() { closesocket(_socket); }

public:
	// Getter
	SOCKET GetSocket() const { return _socket; }
	int    GetSessionId() const { return _sessionId; }

public:
	// Setter
	void SetPosition(int xPos, int yPos)
	{
		_xPos = xPos;
		_yPos = yPos;
	}

private:
	SOCKET	_socket;
	int		_sessionId;
	int		_xPos, _yPos;
};