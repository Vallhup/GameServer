#include "pch.h"
#include "PacketFactory.h"

std::vector<char> PacketFactory::BuildLoginPacket(const GameObject& target)
{
	SC_LOGIN_INFO_PACKET login;
	login.id = target.GetId();
	login.size = sizeof(login);
	login.type = SC_LOGIN_INFO;
	login.x = target.GetX();
	login.y = target.GetY();

	return Serialize(login);
}

std::vector<char> PacketFactory::BuildAddPacket(const GameObject& target)
{
	SC_ADD_OBJECT_PACKET add;
	add.id = target.GetId();
	add.size = sizeof(add);
	add.type = SC_ADD_OBJECT;
	add.x = target.GetX();
	add.y = target.GetY();
	strcpy_s(add.name, NAME_SIZE, target.GetName().c_str());
	add.name[NAME_SIZE - 1] = '\0';

	return Serialize(add);
}

std::vector<char> PacketFactory::BuildMovePacket(const GameObject& target)
{
	SC_MOVE_OBJECT_PACKET move;
	move.id = target.GetId();
	move.size = sizeof(move);
	move.type = SC_MOVE_OBJECT;
	move.x = target.GetX();
	move.y = target.GetY();
	move.move_time = target._lastMoveTime;

	return Serialize(move);
}

std::vector<char> PacketFactory::BuildRemovePacket(const GameObject& target)
{
	SC_REMOVE_OBJECT_PACKET remove;
	remove.id = target.GetId();
	remove.size = sizeof(remove);
	remove.type = SC_REMOVE_OBJECT;

	return Serialize(remove);
}
