#pragma once

enum PacketType : char
{
	// 시스템 관리 (Connect, Disconnect 등)
	PACKET_CONNECT,
	PACKET_ENTER,
	PACKET_LEAVE,

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
	RequestConnectPacket() 
		: PacketHeader(sizeof(RequestConnectPacket), PACKET_CONNECT, 0) {}
};

class ResponseConnectPacket : public PacketHeader
{
public:
	ResponseConnectPacket(char id) 
		: PacketHeader(sizeof(ResponseConnectPacket), PACKET_CONNECT, id), _firstPos{ 4, 4 } {
	}

public:
	Pos _firstPos;
};

class RequestMovePacket : public PacketHeader
{
public:
	RequestMovePacket(char direction) 
		: PacketHeader(sizeof(RequestMovePacket), PACKET_MOVE, 0), _direction(direction) {}

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

class ResponseEnterPacket : public PacketHeader
{
public:
	ResponseEnterPacket(char id, Pos pos)
		: PacketHeader(sizeof(ResponseEnterPacket), PACKET_ENTER, id), _pos(pos) {}

public:
	Pos _pos;
};

class ResponseLeavePacket : public PacketHeader
{
public:
	ResponseLeavePacket(char id)
		: PacketHeader(sizeof(ResponseLeavePacket), PACKET_LEAVE, id) {}
};

#pragma pack(pop)