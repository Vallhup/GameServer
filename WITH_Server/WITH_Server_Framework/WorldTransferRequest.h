#pragma once

#include <vector>

#include "WorldIds.h"
#include "WorldTargetSpec.h"

struct WorldTransferRequest
{
	TransferId id{ 0 };

	std::vector<uint32_t> sessionIds;
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldTargetSpec target;

	PartyId partyId{ 0 };

	bool allowFallback{ false };
	double createdAtSec{ 0.0 };

	inline bool IsValid() const
	{
		return
			!sessionIds.empty() &&
			sourceWorldId.IsValid() &&
			(target.explicitTargetId.has_value() || target.targetWorldDefId.has_value());
	}

	inline uint32_t PlayerCount() const
	{
		return static_cast<uint32_t>(sessionIds.size());
	}
};