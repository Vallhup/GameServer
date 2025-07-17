#include "pch.h"
#include "PacketFactory.h"

std::vector<char> PacketFactory::CSMovePacket(float angle, char direction, bool run)
{
	CS_MOVE_PACKET move;
	move.size = sizeof(move);
	move.type = CS_MOVE;
	move.angle = angle;
	move.direction = direction;
	move.isRun = run;

	return Serialize(move);
}

std::vector<char> PacketFactory::CSAttackPacket(glm::vec3 direction)
{
	CS_ATTACK_PACKET attack;
	attack.size = sizeof(attack);
	attack.type = CS_ATTACK;
	attack.x = direction.x;
	attack.y = direction.y;
	attack.z = direction.z;

	return Serialize(attack);
}

std::vector<char> PacketFactory::CSAttackEndPacket()
{
	CS_ATTACK_END_PACKET attackEnd;
	attackEnd.size = sizeof(attackEnd);
	attackEnd.type = CS_ATTACK_END;

	return Serialize(attackEnd);
}
