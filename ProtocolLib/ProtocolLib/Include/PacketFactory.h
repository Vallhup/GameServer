#pragma once

#include "PacketType.h"
#include "Protocol.h"
#include "SendBuffer.h"

#include "../Protocols/Protocol.pb.h"

template<typename T>
concept ProtoT = std::is_base_of_v<class google::protobuf::MessageLite, T>;

struct PacketFactory {
	template<ProtoT T>
	static SendBuffer* Serialize(PacketType type, const T& data)
	{
		uint16 bodySize = data.ByteSizeLong();
		uint16 packetSize = sizeof(PacketHeader) + bodySize;

		SendBuffer* buffer = SendBufferPool::Get().
			Acquire(bodySize + packetSize);
		buffer->size = packetSize;

		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer->data);
		header->size = packetSize;
		header->type = static_cast<uint16>(type);

		if (not data.SerializeToArray(buffer->data + sizeof(PacketHeader), bodySize))
			throw std::runtime_error("SerializeToArray failed");

		return buffer;
	}

	template<ProtoT T>
	static bool Deserialize(const PacketHeader& header, const BYTE* data, T* out)
	{
		if (header.size < sizeof(PacketHeader)) return false;

		const char* body = reinterpret_cast<const char*>(data) + sizeof(PacketHeader);
		int bodySize = static_cast<int>(header.size - sizeof(PacketHeader));

		return out->ParseFromArray(body, bodySize);
	}

	static bool PeekHeader(const BYTE* data, uint16 size, PacketHeader* out)
	{
		if (size < sizeof(PacketHeader)) return false;

		memcpy(out, data, sizeof(PacketHeader));
		return true;
	}
};