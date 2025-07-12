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
