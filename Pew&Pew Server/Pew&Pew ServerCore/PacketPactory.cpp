#include "pch.h"
#include "PacketPactory.h"

std::vector<char> PacketFactory::CSMovePacket(MoveDirection direction, bool run)
{
	CS_MOVE_PACKET move;
	move.size = sizeof(move);
	move.type = CS_MOVE;
	move.direction = direction;
	move.isRun = run;

	return Serialize(move);
}

std::vector<char> PacketFactory::CSAttackPacket(MoveDirection direction, bool run)
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

	// TODO : Object Status
	move.id = 1;
	move.x = 1;
	move.y = 1;

	return Serialize(move);
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

