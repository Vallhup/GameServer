#pragma once

#include "Protocol.h"
#include "SendBuffer.h"

class ProtobufCodec : public IPacketCodec {
public:
	virtual ~ProtobufCodec() = default;

	virtual bool Encode(uint16 type, const BYTE* payload, uint32 payloadSize,
		SendBuffer& out) override
    {
        if (payloadSize == 0 || payload == nullptr) return false;

        const uint32 totalSize =
            static_cast<uint32>(sizeof(PacketHeader) + payloadSize);

        out.size = totalSize;

        PacketHeader header;
        header.size = totalSize;
        header.type = type;

        std::memcpy(out.data, &header, sizeof(PacketHeader));
        std::memcpy(out.data + sizeof(PacketHeader), payload, payloadSize);

        return true;
    }

	virtual bool Decode(const BYTE* packet, uint32 packetSize,
		PacketHeader& outHeader, const BYTE*& outPayload, uint32& outPayloadSize) override
    {
        if (packetSize < sizeof(PacketHeader)) return false;

        std::memcpy(&outHeader, packet, sizeof(PacketHeader));

        if (outHeader.size != packetSize ||
            outHeader.size < sizeof(PacketHeader)) return false;

        outPayload = packet + sizeof(PacketHeader);
        outPayloadSize = packetSize - sizeof(PacketHeader);

        return true;
    }
};

