#pragma once

#include "WorldDef.h"
#include "WorldId.h"
#include "WorldContentIds.h"
#include "WorldLifecycleEnums.h"

struct WorldInstanceRecord
{
	WorldId id{ WorldId::Invalid() };
	WorldDefId defId{ WorldDefId::None };

	WorldInstanceType instanceType{ WorldInstanceType::Instanced };
	uint64_t instanceKey{ 0 };

	WorldStage stage{ WorldStage::Allocated };


	// [ runtime counters ]
	uint32_t activePlayers{ 0 };
	uint32_t reservedSlots{ 0 };
	uint32_t inFlightTransfersIn{ 0 };
	uint32_t inFlightTransfersOut{ 0 };


	// [ policy cache ]
	JoinPolicy joinPolicy{ JoinPolicy::FreeJoin };
	uint16_t maxPlayerCount{ 0 };
	bool allowReEntry{ false };
	bool destroyWhenEmpty{ false };
	double emptyDestroyDelaySec{ 0.0 };


	// [ lifecycle flags ]
	double emptyElapsedSec{ 0.0 };
	double closingElapsedSec{ 0.0 };

	bool closeRequested{ false };
	bool completionReached{ false };


	inline bool IsRunnable() const
	{
		return stage == WorldStage::Running;
	}

	inline bool IsClosingLike() const
	{
		return
			stage == WorldStage::Closing ||
			stage == WorldStage::DestroyPending ||
			stage == WorldStage::Destroyed ||
			stage == WorldStage::Faulted ||
			closeRequested;
	}

	inline bool IsEmpty() const
	{
		return activePlayers == 0;
	}

	inline uint32_t GetEffectiveTargetOccupancy() const
	{
		return activePlayers + reservedSlots + inFlightTransfersIn;
	}

	inline bool CanReserve(uint32_t requestSlots) const
	{
		if (maxPlayerCount == 0)
			return true;

		return GetEffectiveTargetOccupancy() + requestSlots <= maxPlayerCount;
	}
};