#pragma once

#include <algorithm>
#include <vector>

enum PacketType {
	// 시스템 관리 (0 ~ 9) (Connect, Disconnect 등)
	PACKET_CONNECT = 1,
	PACKET_DISCONNECT = 2,

	// 게임 로직 관리 (10 ~ )
	PACKET_MOVE = 10
};

struct Pos
{
	int x;
	int y;
};

// 실제 전송되는 Data Format
class Packet
{
public:
	Packet() = default;
	Packet(int packetType, int sessionId, size_t dataSize, const char* data);

	virtual ~Packet() = default;

public:
	// Getter
	int		GetType() const { return _packetType; }
	int		GetSessionId() const { return _sessionId; }
	size_t	GetSize() const { return _size; }
	const std::vector<char>& GetData() const { return _data; }

protected:
	int _packetType;
	int _sessionId;
	size_t _size;
	std::vector<char> _data;
};


class MovePacket : public Packet
{
public:

	MovePacket() = default;
	MovePacket(int sessionId, Pos currentPos, Pos targetPos, bool isAvatar);

private:
	Pos _currentPos;
	Pos _targetPos;
	bool _isAvatar;
};