#pragma once

class Packet;

struct Pos
{
	int _xPos;
	int _yPos;
};

// Client의 정보 (고유 id, 연결 socket 등)
// 컨텐츠와 관련된 작업들
class Session
{
public:
	Session() = delete;
	Session(int sessionId, SOCKET s) : _sessionId(sessionId), _socket(s), _pos{1, 1}
	{
	}

	~Session() { closesocket(_socket); }

public:
	void PacketProcessing(Session* session, Packet* packet);
public:
	// Getter
	SOCKET GetSocket() const { return _socket; }
	int    GetSessionId() const { return _sessionId; }
	Pos	   GetPos() const { return _pos; }

public:
	// Setter
	void SetPosition(int moveDirection);

private:
	SOCKET	_socket;
	int		_sessionId;
	Pos		_pos;
};