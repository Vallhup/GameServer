#include "Packet.h"

Packet::Packet(int packetType, int sessionId, size_t dataSize)
	: _size(dataSize), _packetType(packetType), _sessionId(sessionId)
{
}



MovePacket::MovePacket(int sessionId, int direction, bool isAvatar)
	: Packet(PACKET_MOVE, sessionId, sizeof(direction) + sizeof(isAvatar)), 
	_direction(direction), _isAvatar(isAvatar)
{
}
