#include "pch.h"
#include "PacketFactory.h"

std::vector<char> PacketFactory::CSMovePacket(char direction, bool run)
{
	CS_MOVE_PACKET move;
	move.size = sizeof(move);
	move.type = CS_MOVE;
	move.direction = direction;
	move.isRun = run;

	return Serialize(move);
}

std::vector<char> PacketFactory::CSAttackPacket(char direction, bool run)
{
	CS_ATTACK_PACKET attack;
	attack.size = sizeof(attack);
	attack.type = CS_ATTACK;

	return Serialize(attack);
}

std::vector<char> PacketFactory::SCMovePacket()
{
	SC_MOVE_PACKET move;
	move.size = sizeof(move);
	move.type = SC_MOVE_OBJECT;

	// Temp : Object Status
	move.id = 1;
	move.x = 1;
	move.y = 1;

	return Serialize(move);
}

std::vector<char> PacketFactory::SCAddPacket()
{
	SC_ADD_PACKET add;
	add.size = sizeof(add);
	add.type = SC_ADD;
	
	// Temp : Object Status
	add.id = 1;
	add.x = 1;
	add.y = 1;

	return Serialize(add);
}

std::vector<char> PacketFactory::SCRemovePacket()
{
	SC_REMOVE_PACKET remove;
	remove.size = sizeof(remove);
	remove.type = SC_REMOVE;

	// Temp : Object Status
	remove.id = 1;

	return Serialize(remove);
}

std::vector<char> PacketFactory::SCStatUpdatePacket()
{
	SC_STAT_UPDATE_PACKET stat;
	stat.size = sizeof(stat);
	stat.type = SC_STAT_UPDATE;

	// TODO : Object Status
	stat.hp = 1;

	return Serialize(stat);
}

