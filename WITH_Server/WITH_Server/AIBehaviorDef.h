#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

#include "CharacterDef.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "IDs.h"

enum class AIMovementPolicyKind : uint8_t
{
	None,
	Normal
};

enum class AICombatActionPolicyKind : uint8_t
{
	None,
	Imp,
	Weighted
};

enum class AIReactionPolicyKind : uint8_t
{
	None,
	Normal
};

enum class AIIdleActionPolicyKind : uint8_t
{
	None,
	Weighted
};

struct WeightedActionEntry
{
	ActionId actionId{ ActionId::None };
	uint16_t weight{ 0 };
};

struct AIBehaviorProfileDef
{
	AITuningId id{ AITuningIds::None };
	AIArchetype aiType{ AIArchetype::None };

	AIPerceptionTuningComp perceptionTuning{};
	AIDecisionTuningComp decisionTuning{};

	AIMovementPolicyKind movementPolicyKind{ AIMovementPolicyKind::None };
	AICombatActionPolicyKind combatActionPolicyKind{ AICombatActionPolicyKind::None };
	AIIdleActionPolicyKind idleActionPolicyKind{ AIIdleActionPolicyKind::None };
	AIReactionPolicyKind reactionPolicyKind{ AIReactionPolicyKind::None };

	std::span<const WeightedActionEntry> combatActions{};
	std::span<const WeightedActionEntry> idleActions{};
};

struct AIBehaviorDefLoadResult
{
	bool succeeded{ false };
	size_t loadedCount{ 0 };
	std::string error;
};

const AIBehaviorProfileDef* FindAIBehaviorProfileDef(
	AIArchetype aiType,
	AITuningId aiTuningId) noexcept;

std::span<const AIBehaviorProfileDef> GetAIBehaviorProfileDefs() noexcept;

AIBehaviorDefLoadResult LoadAIBehaviorProfileDefsFromJsonDirectory(
	const std::filesystem::path& directory);
