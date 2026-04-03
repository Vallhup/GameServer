#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "NetId.h"

using WorldCommandSourceId = uint32_t;
using WorldCommandTypeKey = uint32_t;

struct WorldCommand
{
	WorldCommandSourceId sourceId{ 0 };
	NetId targetNetId{};
	WorldCommandTypeKey typeKey{ 0 };
	uint32_t sequence{ 0 };
	uint64_t enqueueOrder{ 0 };
	std::vector<std::byte> payload;
};
