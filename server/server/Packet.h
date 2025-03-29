#pragma once

#include <algorithm>
#include <array>

enum PacketType : int {
	// 시스템 관리 (0 ~ 9) (Connect, Disconnect 등)
	PACKET_CONNECT = 1,
	PACKET_DISCONNECT = 2,

	// 게임 로직 관리 (10 ~ )
	PACKET_MOVE = 10
};

enum MoveDirection : int {
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3
};

// 실제 전송되는 Data Format
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
	//const std::array<char, 1024>& GetData() const { return _data; }

protected:
	int _packetType;
	int _sessionId;
	size_t _size;
	//std::array<char, 1024> _data;
};


class MovePacket : public Packet
{
public:
	MovePacket() = default;
	MovePacket(int sessionId, int direction, bool isAvatar);

private:
	int  _direction;
	bool _isAvatar;
};