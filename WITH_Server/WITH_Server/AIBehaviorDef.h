#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "DefLoadResult.h"
#include "DefHash.h"
#include "DefRegistry.h"
#include "CharacterDef.h"
#include "GameplayContentIds.h"

enum class AIActionDistanceBucket : uint8_t
{
	Any,
	VeryClose,
	Close,
	Mid,
	Far
};

struct AIPerceptionTuningDef
{
	double sightRange{ 12.0 };
	double attackRange{ 2.0 };
	double frontDotThreshold{ 0.2 };

	double targetKeepBonus{ 4.0 };
	double lastAttackerBonus{ 2.5 };
	double frontBonus{ 1.0 };
	double switchScoreMargin{ 3.0 };

	double loseSightGraceTime{ 1.2 };
	double leashRange{ 18.0 };
	double hardLeashRange{ 24.0 };
	double leashGaugeMax{ 100.0 };
	double leashDrainPerSec{ 20.0 };
	double hardLeashDrainPerSec{ 60.0 };
	double leashRecoverPerSec{ 35.0 };
	double returnHomeArriveRange{ 0.8 };
	double returnHomeReaggroLockSec{ 1.5 };
	double returnHpRegenPerSecRatio{ 0.08 };
	double idleActionCooldownSec{ 6.0 };
	int idleActionChancePercent{ 25 };
	double assistRange{ 6.0 };
};

struct AIDecisionTuningDef
{
	double decisionInterval{ 0.2 };
	double attackCooldown{ 1.8 };
	double reactDuration{ 0.5 };
};

struct WeightedActionEntry
{
	AbilityId abilityId{ InvalidAbilityId };
	uint16_t weight{ 0 };
	float aiCooldownSec{ 0.0f };
	AIActionDistanceBucket distanceBucket{ AIActionDistanceBucket::Any };
	bool forbidImmediateRepeat{ false };
};

struct AIBossPhaseTransitionDef
{
	uint8_t phase{ 1 };
	float hpRatio{ 1.0f };
	AbilityId transitionAbilityId{ InvalidAbilityId };
	float transitionLockSec{ 0.0f };
};

struct AIBossMovementTuningDef
{
	double veryCloseDistance{ 2.0 };
	double closeDistance{ 5.8 };
	double midDistance{ 7.5 };
	double preferredMinDistance{ 3.2 };
	double preferredMaxDistance{ 6.8 };
	float strafeMinSec{ 0.7f };
	float strafeMaxSec{ 1.5f };
};

struct AIBossPatternProfileDef
{
	AIBossMovementTuningDef movement{};
	std::span<const AIBossPhaseTransitionDef> phaseTransitions{};
};

struct AIBehaviorProfileDef
{
	AIBehaviorProfileId id{ InvalidAIBehaviorProfileId };
	std::string key;
	AIArchetype aiType{ AIArchetype::None };

	AIPerceptionTuningDef perception{};
	AIDecisionTuningDef decision{};

	std::span<const WeightedActionEntry> combatActions{};
	std::span<const WeightedActionEntry> idleActions{};

	AIBossPatternProfileDef bossPattern{};
};

struct AIBehaviorProfileTraits
{
	static AIBehaviorProfileId GetId(
		const AIBehaviorProfileDef& def) noexcept
	{
		return def.id;
	}
};

using AIBehaviorProfileDefRegistry = DefRegistry<
	AIBehaviorProfileDef,
	AIBehaviorProfileId,
	AIBehaviorProfileTraits>;

struct AIBehaviorDefinitionSet
{
	AIBehaviorProfileDefRegistry profiles;
	std::vector<std::vector<WeightedActionEntry>> combatActionStorage;
	std::vector<std::vector<WeightedActionEntry>> idleActionStorage;
	std::vector<std::vector<AIBossPhaseTransitionDef>> bossPhaseTransitionStorage;

	void Clear() noexcept
	{
		profiles.Clear();
		combatActionStorage.clear();
		idleActionStorage.clear();
		bossPhaseTransitionStorage.clear();
	}

	size_t Size() const noexcept
	{
		return profiles.Size();
	}
};

using AIBehaviorDefLoadResult = DefLoadResult;

AIBehaviorDefLoadResult LoadAIBehaviorProfileDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AIBehaviorDefinitionSet& outDefs);
