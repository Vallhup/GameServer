#pragma once

#include "Protocols/Protocol.pb.h"

class PacketFactory {
public:
	static std::vector<char> CSLoginPacket();
	static std::vector<char> CSInputPacket(Protocol::Input key, Protocol::InputType type);

public:
	static std::vector<char> SCLoginPacket(int id);
	static std::vector<char> SCAddPacket(int id, const Protocol::Vec3& pos);
	static std::vector<char> SCMovePakcet(int id, const Protocol::Vec3& pos);
	static std::vector<char> SCRemovePacket(int id);
};