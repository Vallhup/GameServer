#pragma once

#include <cstdint>
#include <optional>

#include "WorldDef.h"
#include "WorldId.h"

struct WorldTargetSpec
{
	std::optional<WorldId> explicitTargetId;
	std::optional<WorldDefId> targetWorldDefId;
	uint64_t instanceKey{ 0 };
};

