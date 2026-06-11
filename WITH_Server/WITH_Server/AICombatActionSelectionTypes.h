#pragma once

#include "AIBehaviorDef.h"
#include "IAIState.h"

enum class AIImmediateRepeatMode : uint8_t
{
	ForbidConfiguredRepeats,
	AllowAsFallback
};

struct AICombatActionCandidate
{
	size_t actionIndex{ 0 };
	uint32_t weight{ 0 };
};

struct AICombatActionChoice
{
	size_t actionIndex{ std::numeric_limits<size_t>::max() };

	bool IsValid(size_t actionCount) const noexcept
	{
		return actionIndex < actionCount;
	}
};

struct AICombatActionSelectionContext
{
	const AIContext& aiCtx;
	AIActionRuntimeComp* runtime{ nullptr };
	std::span<const AIActionDef> actions;

	const AIMovementProfileDef* movement{ nullptr };
	AIActionDistanceBucket currentBucket{ AIActionDistanceBucket::Any };
	double distanceToTarget{ 0.0 };

	uint8_t phase{ 1 };
	float selfHpRatio{ 1.0f };
	float targetHpRatio{ 1.0f };
	AbilityId lastUsedAbilityId{ InvalidAbilityId };
	uint32_t actionSequence{ 0 };
	uint16_t basicActionCountSinceEffect{ 0 };
};
