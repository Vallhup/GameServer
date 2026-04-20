#include "pch.h"
#include "ClientInboundPacketQueue.h"

void ClientInboundPacketQueue::Push(const PacketHeader& header, const uint8_t* data)
{
	if (data == nullptr ||
		header.size < sizeof(PacketHeader))
	{
		return;
	}

	ClientInboundPacket packet;
	packet.header = header;
	packet.bytes.assign(data, data + header.size);
	{
		std::lock_guard lock{ _mtx };
		_packets.push_back(std::move(packet));
	}
}

void ClientInboundPacketQueue::Drain(std::vector<ClientInboundPacket>& out)
{
	out.clear();
	{
		std::lock_guard lock{ _mtx };
		out.swap(_packets);
	}
}
