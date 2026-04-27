#pragma once

#include "GameplayComponentPrerequisites.h"

struct ReplicationStatsComp : Component
{
	uint64_t droppedUnsupportedCommandTypeCount{ 0 };
	uint64_t droppedInvalidPayloadCount{ 0 };
	uint64_t droppedTargetMissingCount{ 0 };
	uint64_t ownerMismatchCount{ 0 };
	uint64_t deferredPotionEventCount{ 0 };
	uint64_t missingAnimationRegistryCount{ 0 };
	uint64_t missingWorldTransferPayloadCount{ 0 };
	uint64_t replicationTodoSkippedCount{ 0 };
};
