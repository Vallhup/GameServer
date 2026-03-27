#pragma once

#include "WorldId.h"
#include "WorldContentIds.h"
#include "WorldLifecycleEnums.h"

struct WorldInstanceRecord
{
	WorldId id{ WorldId::Invalid() };
	WorldDefId defId{ WorldDefId::None };

	WorldLifetimeKind lifetime{ WorldLifetimeKind::Instanced };
	uint64_t instanceKey{ 0 };

	WorldStage stage{ WorldStage::Allocated };

	uint32_t activePlayers{ 0 };
	uint32_t reservedSlots{ 0 };
	uint32_t inFlightTransfers{ 0 };

	double emptyElapsedSec{ 0.0 };
	bool closeRequested{ false };
};