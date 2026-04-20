#pragma once

struct ClientInboundPacket
{
	PacketHeader header;
	std::vector<uint8_t> bytes;
};

class ClientInboundPacketQueue {
public:
	void Push(const PacketHeader& header, const uint8_t* data);
	void Drain(std::vector<ClientInboundPacket>& out);

private:
	std::mutex _mtx;
	std::vector<ClientInboundPacket> _packets;
};
