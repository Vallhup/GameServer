#pragma once

#include <vector>

#include "Protocols/Protocol.pb.h"

#pragma pack(push, 1)

struct PacketHeader {
	uint16_t size;
	uint16_t type;
};

#pragma pack(pop)

enum class PacketType : uint16_t {
	CS_LOGIN,
	CS_MOVE,

	SC_LOGIN,
	SC_MOVE_OBJECT,
	SC_ADD,
	SC_REMOVE
};

template<typename T>
concept ProtoT = std::is_base_of_v<google::protobuf::MessageLite, T>;

struct PacketFactory {
	template<ProtoT T>
	static std::vector<char> Serialize(PacketType type, const T& data)
	{
		uint16_t bodySize = data.ByteSizeLong();
		uint16_t packetSize = sizeof(PacketHeader) + bodySize;

		std::vector<char> buffer(packetSize);

		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer.data());
		header->size = packetSize;
		header->type = static_cast<uint16_t>(type);

		if (not data.SerializeToArray(buffer.data() + sizeof(PacketHeader), bodySize))
			throw std::runtime_error("SerializeToArray failed");

		return buffer;
	}

	template<ProtoT T>
	static bool Deserialize(const PacketHeader* header, T* out)
	{
		if (header->size < sizeof(PacketHeader)) return false;

		const char* body = reinterpret_cast<const char*>(header) + sizeof(PacketHeader);
		int bodySize = static_cast<int>(header->size - sizeof(PacketHeader));

		return out->ParseFromArray(body, bodySize);
	}

	static bool PeekHeader(const char* data, uint16_t size, PacketHeader* out)
	{
		if (size < sizeof(PacketHeader)) return false;

		memcpy(out, data, sizeof(PacketHeader));
		return true;
	}
};