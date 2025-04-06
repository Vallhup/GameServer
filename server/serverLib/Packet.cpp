#include "pch.h"
#include "Packet.h"

Packet::Packet()
	: _size(0), _packetType(0), _sessionId(0)
{
}

Packet::Packet(int packetType, int sessionId, int dataSize)
	: _size(dataSize), _packetType(packetType), _sessionId(sessionId)
{
}

ConnectPacket::ConnectPacket() : Packet(PACKET_CONNECT, 0, sizeof(ConnectPacket)), _isAvatar(true), _startPos{0, 0}
{
}

ConnectPacket::ConnectPacket(int sessionId, Pos startPos, bool isAvatar)
	:Packet(PACKET_CONNECT, sessionId, sizeof(ConnectPacket)), _isAvatar(isAvatar), _startPos{ startPos._xPos, startPos._yPos }
{
}

DisconnectPacket::DisconnectPacket() : Packet(PACKET_DISCONNECT, 0, sizeof(DisconnectPacket))
{
}

MovePacket::MovePacket(int sessionId, int direction)
	: Packet(PACKET_MOVE, sessionId, sizeof(MovePacket)), _direction(direction), _isAvatar(false), _pos{0, 0}
{
}

MovePacket::MovePacket(int sessionId, Pos pos, bool isAvatar)
	: Packet(PACKET_MOVE, sessionId, sizeof(MovePacket)), _direction(0), _isAvatar(isAvatar), _pos(pos)
{
}