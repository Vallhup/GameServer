#pragma once

#include "PacketFactory.h"

namespace NetHelper
{
	template <ProtoT T, typename Fn>
	void DispatchPacket(const PacketHeader& header, const BYTE* data, Fn&& fn)
	{
		T packet;
		if (PacketFactory::Deserialize<T>(header, data, &packet))
			fn(packet);
	}
}