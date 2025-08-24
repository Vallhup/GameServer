#pragma once

#include "Protocols/Protocol.pb.h"

class PacketFactory {
public:
	static std::vector<char> CSInputPacket(Protocol::Input key, Protocol::InputType type);

public:
	static std::vector<char> SCMovePakcet();
};

