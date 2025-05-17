#pragma once

#include "Macro.h"

class PacketFactory
{
public:
	static std::vector<char> BuildLoginPacket(const GameObject& target);
	static std::vector<char> BuildAddPacket(const GameObject& target);
	static std::vector<char> BuildMovePacket(const GameObject& target);
	static std::vector<char> BuildRemovePacket(const GameObject& target);

public:
	template<typename Packet>
	static std::vector<char> Serialize(Packet& packet)
	{
		static_assert(std::is_trivially_copyable_v<Packet>);

		std::vector<char> out(sizeof(Packet));
		std::memcpy(out.data(), &packet, sizeof(Packet));

		return out;
	}

	template<typename Packet>
	static Packet Deserialize(const std::vector<char>& buf)
	{
		static_assert(std::is_trivially_copyable_v<Packet>);

		if (buf.size() < sizeof(Packet)) {
			LOG_ERR("Deserialize Packet Size Error");
			return Packet{};
		}

		Packet packet{};
		std::memcpy(&packet, buf.data(), sizeof(Packet));

		return packet;
	}
};

