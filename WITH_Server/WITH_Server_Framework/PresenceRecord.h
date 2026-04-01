#pragma once

#include "WorldId.h"
#include "WorldIds.h"
#include "WorldLifecycleEnums.h"

struct PresenceRecord
{
	PresenceId id{ 0 };

	uint32_t sessionId{ 0 };

	PresenceStage state{ PresenceStage::None };

	WorldId currentWorldId{ WorldId::Invalid() };
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldId targetWorldId{ WorldId::Invalid() };

	TransferId activeTransferId{ 0 };

	bool disconnected{ false };
	bool reEntryEligible{ false };

	double updatedAtSec{ 0.0 };

	inline bool IsTransfering() const
	{
		return activeTransferId != 0;
	}

	inline bool HasCurrentWorld() const
	{
		return currentWorldId.IsValid();
	}

	inline bool IsActiveInWorld(WorldId worldId) const
	{
		return
			state == PresenceStage::Active &&
			currentWorldId == worldId;
	}

	inline bool CanReEnterWorld(WorldId targetWorldId) const
	{
		return
			disconnected &&
			reEntryEligible &&
			currentWorldId == targetWorldId &&
			!IsTransfering();
	}
};