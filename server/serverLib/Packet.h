#pragma once

#include "pch.h"

enum PacketType : int {
	// 矫胶袍 包府 (0 ~ 9) (Connect, Disconnect 殿)
	PACKET_CONNECT = 1,
	PACKET_DISCONNECT = 2,

	// 霸烙 肺流 包府 (10 ~ )
	PACKET_MOVE = 10
};

enum MoveDirection : int {
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3
};

// 角力 傈价登绰 Data Format
class Packet
{
public:
	Packet() = default;
	Packet(int packetType, int sessionId, size_t dataSize);

	virtual ~Packet() = default;

public:
	// Getter
	int		GetType() const { return _packetType; }
	int		GetSessionId() const { return _sessionId; }
	size_t	GetSize() const { return _size; }

protected:
	int _packetType;
	int _sessionId;
	size_t _size;
};

class ConnectPacket : public Packet
{
public:
	// Client 积己磊
	ConnectPacket();

	// Server 积己磊
	ConnectPacket(int sessionId, Pos startPos);

public:
	Pos GetPos() const { return _startPos; }

private:
	Pos _startPos;

};

class DisconnectPacket : public Packet
{
public:
	DisconnectPacket();

};

class MovePacket : public Packet
{
public:
	MovePacket() = default;

	// Client 积己磊
	MovePacket(int sessionId, int direction);

	// Server 积己磊
	MovePacket(int sessionId, Pos pos, bool isAvatar);


public:
	int GetDirection() const { return _direction; }
	Pos GetPos() const { return _pos; }

private:
	int  _direction;
	Pos  _pos;
	bool _isAvatar;
};