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

std::vector<char> PacketFactory::SCMovePacket(const Character& character)
{
	SC_MOVE_PACKET move;
	move.size = sizeof(move);
	move.type = SC_MOVE_OBJECT;
	move.id = character.GetId();

	vec3 charPos = character.GetPosition();

	move.x = charPos.x;
	move.y = charPos.y;
	move.z = charPos.z;

	return Serialize(move);
}

std::vector<char> PacketFactory::SCAddPacket(const Character& character)
{
	SC_ADD_PACKET add;
	add.size = sizeof(add);
	add.type = SC_ADD;
	add.id = character.GetId();

	vec3 charPos = character.GetPosition();

	add.x = charPos.x;
	add.y = charPos.y;
	add.z = charPos.z;

	return Serialize(add);
}

std::vector<char> PacketFactory::SCRemovePacket(const Character& character)
{
	SC_REMOVE_PACKET remove;
	remove.size = sizeof(remove);
	remove.type = SC_REMOVE;
	remove.id = character.GetId();

	return Serialize(remove);
}

std::vector<char> PacketFactory::SCStatUpdatePacket(const Character& character)
{
	SC_STAT_UPDATE_PACKET stat;
	stat.size = sizeof(stat);
	stat.type = SC_STAT_UPDATE;
	stat.hp = character.GetHp();

	return Serialize(stat);
}

