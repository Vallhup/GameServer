#pragma once

enum PacketType : char
{
	// 시스템 관리 (Connect, Disconnect 등)
	PACKET_CONNECT,
	PACKET_DISCONNECT,

	// 게임 로직 관리
	PACKET_MOVE
};

enum MoveDirection : char {
	MOVE_UP,
	MOVE_DOWN,
	MOVE_LEFT,
	MOVE_RIGHT
};

#pragma pack(push, 1)
class PacketHeader
{
public:
	PacketHeader(char size, char type, char id) : _size(size), _type(type), _id(id) {}

public:
	char _size;
	char _type;
	int  _id;
};

class RequestConnectPacket : public PacketHeader
{
public:
	RequestConnectPacket(char id) 
		: PacketHeader(sizeof(RequestConnectPacket), PACKET_CONNECT, id) {}
};

class ResponseConnectPacket : public PacketHeader
{
public:
	ResponseConnectPacket(char id) 
		: PacketHeader(sizeof(ResponseConnectPacket), PACKET_CONNECT, id), _firstPos(1, 1) {}

public:
	Pos _firstPos;
};

class RequestMovePacket : public PacketHeader
{
public:
	RequestMovePacket(char id, char direction) 
		: PacketHeader(sizeof(RequestMovePacket), PACKET_MOVE, id), _direction(direction) {}

public:
	char _direction;
};

class ResponseMovePacket : public PacketHeader
{
public:
	ResponseMovePacket(char id, Pos pos) 
		: PacketHeader(sizeof(ResponseMovePacket), PACKET_MOVE, id), _pos(pos) {}

public:
	Pos _pos;
};

char* Serialization(PacketHeader* packet)
{
	char* temp = new char[packet->_size];

	std::memcpy(temp, packet, packet->_size);

	return temp;
}
#pragma pack(pop)