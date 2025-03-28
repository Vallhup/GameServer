#include "Packet.h"

Packet::Packet(int packetType, int sessionId, size_t dataSize, const char* data)
	: _size(dataSize), _packetType(packetType), _sessionId(sessionId)
{
	_data.resize(dataSize);
	std::copy(data, data + dataSize, _data.begin());
}

MovePacket::MovePacket(int sessionId, Pos currentPos, Pos targetPos, bool isAvatar)
	: Packet(PACKET_MOVE, sessionId, sizeof(_currentPos) + sizeof(_targetPos) + sizeof(_isAvatar), NULL),
	_currentPos(currentPos), _targetPos(targetPos), _isAvatar(isAvatar)
{
	_data.resize(sizeof(_currentPos) + sizeof(_targetPos) + sizeof(_isAvatar));

	std::memcpy(_data.data(), &_currentPos, sizeof(_currentPos));
	std::memcpy(_data.data() + sizeof(_currentPos), &_targetPos, sizeof(_targetPos));
	std::memcpy(_data.data() + sizeof(_currentPos) + sizeof(_targetPos), &_isAvatar, sizeof(_isAvatar));
}
