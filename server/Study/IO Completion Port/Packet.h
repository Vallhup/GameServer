#pragma once

enum PacketType : char
{
	REQUEST_CONNECT,
	RESPONSE_CONNECT,
	REQUEST_MOVE,
	RESPONSE_MOVE,
	RESPONSE_ENTER,
	RESPONSE_LEAVE
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

	std::vector<char> Serialize() const
	{
		std::vector<char> outVector(_size);
		std::memcpy(outVector.data(), this, _size);
		return outVector;
	}

public:
	char _size;
	char _type;
	int  _id;
};

class RequestConnectPacket : public PacketHeader
{
public:
	RequestConnectPacket() 
		: PacketHeader(sizeof(RequestConnectPacket), REQUEST_CONNECT, 0) {}
};

class ResponseConnectPacket : public PacketHeader
{
public:
	ResponseConnectPacket(char id) 
		: PacketHeader(sizeof(ResponseConnectPacket), RESPONSE_CONNECT, id), _firstPos{ 4, 4 } {
	}

public:
	Pos _firstPos;
};

class RequestMovePacket : public PacketHeader
{
public:
	RequestMovePacket(char direction) 
		: PacketHeader(sizeof(RequestMovePacket), REQUEST_MOVE, 0), _direction(direction) {}

public:
	char _direction;
};

class ResponseMovePacket : public PacketHeader
{
public:
	ResponseMovePacket(char id, Pos pos) 
		: PacketHeader(sizeof(ResponseMovePacket), RESPONSE_MOVE, id), _pos(pos) {}

public:
	Pos _pos;
};

class ResponseEnterPacket : public PacketHeader
{
public:
	ResponseEnterPacket(char id, Pos pos)
		: PacketHeader(sizeof(ResponseEnterPacket), RESPONSE_ENTER, id), _pos(pos) {}

public:
	Pos _pos;
};

class ResponseLeavePacket : public PacketHeader
{
public:
	ResponseLeavePacket(char id)
		: PacketHeader(sizeof(ResponseLeavePacket), RESPONSE_LEAVE, id) {}
};

#pragma pack(pop)

class PacketFactory
{
public:
	static PacketHeader* CreatePacket(const std::vector<char>& data)
	{
		char dataSize = data[0];
		char dataType = data[1];

		if ((data.size() < sizeof(PacketHeader)) or (data.size() < dataSize)) {
			std::cout << "PacketFactory PacketSize Error\n";
			return nullptr;
		}

		PacketHeader dummyPacket(0, 0, 0);
		std::memcpy(&dummyPacket, data.data(), sizeof(PacketHeader));

		switch (dataType) {
		case REQUEST_CONNECT: {
			RequestConnectPacket* packet = new RequestConnectPacket;
			std::memcpy(packet, data.data(), dataSize);
			return packet;
		}

		case RESPONSE_CONNECT: {
			ResponseConnectPacket* packet = new ResponseConnectPacket(dummyPacket._id);
			std::memcpy(packet, data.data(), dataSize);
			return packet;
		}
		case REQUEST_MOVE: {
			RequestMovePacket* packet = new RequestMovePacket(MOVE_UP);
			std::memcpy(packet, data.data(), dataSize);
			return packet;
		}
		case RESPONSE_MOVE: {
			ResponseMovePacket* packet = new ResponseMovePacket(dummyPacket._id, { 0, 0 });
			std::memcpy(packet, data.data(), dataSize);
			return packet;
		}
		case RESPONSE_ENTER: {
			ResponseEnterPacket* packet = new ResponseEnterPacket(dummyPacket._id, { 0, 0 });
			std::memcpy(packet, data.data(), dataSize);
			return packet;
		}
		case RESPONSE_LEAVE: {
			ResponseLeavePacket* packet = new ResponseLeavePacket(dummyPacket._id);
			std::memcpy(packet, data.data(), dataSize);
			return packet;
		}
		default:
			std::cout << "PacketFactory Error\n";
			return nullptr;
		}
	}
};