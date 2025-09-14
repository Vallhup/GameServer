#pragma once

#include "Protocols/Protocol.pb.h"

class PacketFactory {
public:
	static std::vector<char> CSLoginPacket();
	static std::vector<char> CSMovePacket(int id, bool dir[4], float yaw, float pitch);
	static std::vector<char> CSAttackPacket(int id);
	static std::vector<char> CSDodgePacket(int id);

public:
	static std::vector<char> SCLoginPacket(int id);
	static std::vector<char> SCAddPacket(int id, const Protocol::Vec3& pos);
	static std::vector<char> SCMovePakcet(int id, const Protocol::Vec3& pos, float rot);
	static std::vector<char> SCRemovePacket(int id);
	static std::vector<char> SCAttackPacket(int id, float rot);
	static std::vector<char> SCDodgePacket(int id, const Protocol::Vec3& dir);
};