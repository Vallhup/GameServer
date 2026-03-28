#pragma once

#include "WorldId.h"
#include "WorldLifecycleEnums.h"

using PresenceId = uint64_t;
using TransferId = uint64_t;

struct PResenceRecord
{
	PresenceId id{ 0 };

	uint32_t connectionId{ 0 };

	PresenceStage state{ PresenceStage::None };

	WorldId currentWorldId{ WorldId::Invalid() };
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldId targetWorldId{ WorldId::Invalid() };

	TransferId activeTransferId{ 0 };

	bool disconnected{ false };
	bool reentryEligible{ false };

	inline bool IsTransfering() const
	{
		return activeTransferId != 0;
	}

	inline bool HasCurrentWorld() const
	{
		return currentWorldId.IsValid();
	}
};