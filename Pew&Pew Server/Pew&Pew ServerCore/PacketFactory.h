#pragma once

class PacketFactory
{
public:
	// Client -> Server
	static std::vector<char> CSMovePacket(char direction, bool run = false);
	static std::vector<char> CSAttackPacket(char direction, bool run = false);

public:
	// Server -> Client
	static std::vector<char> SCMovePacket();
	static std::vector<char> SCAddPacket();
	static std::vector<char> SCRemovePacket();
	static std::vector<char> SCStatUpdatePacket();

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
