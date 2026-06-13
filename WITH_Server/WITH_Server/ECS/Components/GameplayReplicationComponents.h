#pragma once

#include "GameplayComponentPrerequisites.h"
#include "../../GameplayContentIds.h"

struct GameplayEffectAppliedReplicationEvent
{
	GameplayEffectId effectId{ InvalidGameplayEffectId };
	uint32_t instanceId{ 0 };
	uint16_t stackCount{ 1 };
	float remainingDurationSec{ 0.0f };
};

struct GameplayEffectReplicationComp : Component
{
	uint64_t revision{ 0 };
	std::vector<GameplayEffectAppliedReplicationEvent> pendingAppliedEffects;
};

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
