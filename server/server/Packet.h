#pragma once

struct Pos
{
	int x;
	int y;
};

// Packet Header
// 1. Packet Size
// 2. Packet Id
// 3. Client Id or Session Id
class Packet
{
public:
	Packet() = delete;
	Packet(size_t size, unsigned long long id, unsigned long long sessionId);

protected:
	size_t _size;
	unsigned long long _id;
	unsigned long long _sessionId;

};

class MovePacket : public Packet
{
public:
	MovePacket() = delete;
	MovePacket(Pos currentPos, Pos targetPos, bool isAvatar);

private:
	// 1. Current Pos
	// 2. Target Pos
	// 3. isAvatar
	Pos _currentPos;
	Pos _targetPos;
	bool _isAvatar;

};