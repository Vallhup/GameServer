#pragma once

#pragma pack(push, 1)

struct PacketHeader {
	uint32 size;
	uint16 type;
};

#pragma pack(pop)

struct SendBuffer;

class IPacketCodec {
public:
	virtual ~IPacketCodec() = default;

	virtual bool Encode(uint16 type, const BYTE* payload, uint32 payloadSize,
		SendBuffer& out) = 0;

	virtual bool Decode(const BYTE* packet, uint32 packetSize,
		PacketHeader& outHeader, const BYTE*& outPayload, uint32& outPayloadSize) = 0;
};