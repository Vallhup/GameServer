#pragma once

#include "Macro.h"

class GameObject;
class Party;


class PacketFactory
{
public:
	// Login, Move
	static std::vector<char> BuildLoginPacket(const GameObject& target);
	static std::vector<char> BuildAddPacket(const GameObject& target);
	static std::vector<char> BuildMovePacket(const GameObject& target);
	static std::vector<char> BuildRemovePacket(const GameObject& target);
	static std::vector<char> BuildStatChangePacket(const GameObject& target);

public:
	// Party
	static std::vector<char> BuildPartyRequestPacket(int fromId);
	static std::vector<char> BuildPartyResultPacket(int targetId, bool accept);
	static std::vector<char> BuildPartyUpdatePacket(const Party& party);
	static std::vector<char> BuildPartyDisbandPacket(const Party& party);

public:
	// Item
	static std::vector<char> BuildAddItemPacket(char itemId, int count);
	static std::vector<char> BuildUseItemOkPacket(char itemId);

public:
	template<typename Packet>
	static std::vector<char> Serialize(const Packet& packet)
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