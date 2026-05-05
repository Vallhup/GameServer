#pragma once

#include <cstdint>
#include "DynamicTaskTypes.h"

template<uint32_t MaxPacketTypes = 1024>
class PacketDispatchTable {
public:
	PacketDispatchTable() { _table.fill(InvalidDynamicTaskTypeId); }

	PacketDispatchTable(const PacketDispatchTable&)				= delete;
	PacketDispatchTable& operator=(const PacketDispatchTable&)	= delete;

	void Register(uint16_t packetType, DynamicTaskTypeId id);
	DynamicTaskTypeId Lookup(uint16_t packetType) const noexcept;

private:
	std::array<DynamicTaskTypeId, MaxPacketTypes> _table;
};

template<uint32_t MaxPacketTypes>
inline void PacketDispatchTable<MaxPacketTypes>::Register(uint16_t packetType, DynamicTaskTypeId id)
{
	assert(packetType < MaxPacketTypes);
	_table[packetType] = id;
}

template<uint32_t MaxPacketTypes>
inline DynamicTaskTypeId PacketDispatchTable<MaxPacketTypes>::Lookup(uint16_t packetType) const noexcept
{
	if (packetType >= MaxPacketTypes) return InvalidDynamicTaskTypeId;
	return _table[packetType];
}

using DefaultPacketDispatchTable = PacketDispatchTable<1024>;
