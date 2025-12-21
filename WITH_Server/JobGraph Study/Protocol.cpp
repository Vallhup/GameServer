#include "Protocol.hpp"

SendBuffer PacketFactory::SCLoginPacket(int sessionId)
{
	Protocol::SC_LOGIN_PACKET login;
	login.set_sessionid(sessionId);

	return Serialize<Protocol::SC_LOGIN_PACKET>(PacketType::SC_LOGIN, login);
}

SendBuffer PacketFactory::SCAddPacket(int sessionId, float x, float y, float z)
{
	Protocol::SC_ADD_PACKET add;
	add.set_sessionid(sessionId);
	add.set_x(x);
	add.set_y(y);
	add.set_z(z);

	return Serialize<Protocol::SC_ADD_PACKET>(PacketType::SC_ADD, add);
}

SendBuffer PacketFactory::SCMovePacket(int sessionId, float x, float y, float z, float yaw)
{
	Protocol::SC_MOVE_PACKET move;
	move.set_sessionid(sessionId);
	move.set_x(x);
	move.set_y(y);
	move.set_z(z);
	move.set_yaw(yaw);

	return Serialize<Protocol::SC_MOVE_PACKET>(PacketType::SC_MOVE_OBJECT, move);
}
