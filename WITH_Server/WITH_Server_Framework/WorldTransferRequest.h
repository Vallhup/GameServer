#pragma once

#include <vector>

#include "WorldTargetSpec.h"

struct WorldTransferRequest
{
	std::vector<uint32_t> connectionIds;
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldTargetSpec target;
};