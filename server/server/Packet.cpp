#include "Packet.h"

Packet::Packet(int packetType, int sessionId, size_t dataSize)
	: _size(dataSize), _packetType(packetType), _sessionId(sessionId)
{
}

ConnectPacket::ConnectPacket() : Packet(PACKET_CONNECT, 0, 0)
{
	_startPos._xPos = 0;
	_startPos._yPos = 0;
}

ConnectPacket::ConnectPacket(int sessionId, Pos startPos)
	:Packet(PACKET_CONNECT, sessionId, sizeof(ConnectPacket) - sizeof(Packet))
{
	_startPos._xPos = startPos._xPos;
	_startPos._yPos = startPos._yPos;
}

DisconnectPacket::DisconnectPacket() : Packet(PACKET_DISCONNECT, 0, 0)
{
}

MovePacket::MovePacket(int sessionId, int direction)
	: Packet(PACKET_MOVE, sessionId, sizeof(MovePacket) - sizeof(Packet)), _direction(direction), _isAvatar(false)
{
	_pos._xPos = 0;
	_pos._yPos = 0;
}

MovePacket::MovePacket(int sessionId, Pos pos, bool isAvatar)
	: Packet(PACKET_MOVE, sessionId, sizeof(MovePacket) - sizeof(Packet)), _direction(0), _isAvatar(isAvatar)
{
	_pos = pos;
}
