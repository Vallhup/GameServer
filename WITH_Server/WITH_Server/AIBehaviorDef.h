#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
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

enum class AIMovementBehavior : uint8_t
{
	None,
	Hold,
	Approach,
	RunApproach,
	Retreat,
	Strafe,
	CircleLeft,
	CircleRight,
	SearchLastKnown,
	ReturnHome
};

enum class AIReactionRuleEvent : uint8_t
{
	OnHitReceived,
	OnParried,
	OnGuardBroken,
	OnHpThreshold
};

enum class AIReactionRuleOutcome : uint8_t
{
	Ignore,
	ForceRetarget,
	EnterReact,
	IssueAbility,
	EnterReactAndIssueAbility
};

using AIActionGroupId = uint16_t;
inline constexpr AIActionGroupId InvalidAIActionGroupId = 0;

struct AIPerceptionTuningDef
{
	double sightRange{ 12.0 };
	double attackRange{ 2.0 };
	double frontDotThreshold{ 0.2 };

	// Deprecated targeting fallback. Prefer AIBehaviorProfileDef::targeting.
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

struct AITargetingTuningDef
{
	double targetKeepBonus{ 4.0 };
	double lastAttackerBonus{ 2.5 };
	double frontBonus{ 1.0 };
	double switchScoreMargin{ 3.0 };
	bool preferNearest{ true };
	bool preferLowestHp{ false };
	double assistRange{ 6.0 };
};

struct AIDecisionTuningDef
{
	double decisionInterval{ 0.2 };
	double attackCooldown{ 1.8 };
	double reactDuration{ 0.5 };
};

struct AIActionConditionDef
{
	uint8_t phaseMin{ 1 };
	uint8_t phaseMax{ 255 };
	AIActionDistanceBucket distanceBucket{ AIActionDistanceBucket::Any };
	std::optional<float> minDistance;
	std::optional<float> maxDistance;
	float selfHpRatioMin{ 0.0f };
	float selfHpRatioMax{ 1.0f };
	float targetHpRatioMin{ 0.0f };
	float targetHpRatioMax{ 1.0f };
	bool requiresTargetVisible{ true };
	bool requiresTargetInFront{ false };
	bool requiresAbilityAvailable{ true };
};

struct AIActionDef
{
	AbilityId abilityId{ InvalidAbilityId };
	uint16_t weight{ 0 };
	AIActionConditionDef condition{};
	float aiCooldownSec{ 0.0f };
	float globalCooldownSec{ 0.0f };
	AIActionGroupId groupId{ InvalidAIActionGroupId };
	float groupCooldownSec{ 0.0f };
	bool forbidImmediateRepeat{ false };
	float repeatWeightMultiplier{ 0.35f };
	float lockMovementSec{ 0.0f };
	bool lockFacingToTarget{ false };
	int chancePercent{ 100 };
};

struct AIPhaseTransitionDef
{
	std::optional<uint8_t> fromPhase;
	uint8_t toPhase{ 1 };
	float hpRatio{ 1.0f };
	AbilityId transitionAbilityId{ InvalidAbilityId };
	float transitionLockSec{ 0.0f };
	bool clearActionCooldowns{ false };
	bool clearGroupCooldowns{ false };
	bool forceRetarget{ false };
};

struct AIMovementProfileDef
{
	std::string name;
	uint8_t phaseMin{ 1 };
	uint8_t phaseMax{ 255 };

	double veryCloseDistance{ 0.0 };
	double closeDistance{ 0.0 };
	double midDistance{ 0.0 };
	double farDistance{ 0.0 };
	double preferredMinDistance{ 0.0 };
	double preferredMaxDistance{ 0.0 };

	AIMovementBehavior veryCloseBehavior{ AIMovementBehavior::Retreat };
	AIMovementBehavior closeBehavior{ AIMovementBehavior::Hold };
	AIMovementBehavior preferredBehavior{ AIMovementBehavior::Hold };
	AIMovementBehavior midBehavior{ AIMovementBehavior::Approach };
	AIMovementBehavior farBehavior{ AIMovementBehavior::RunApproach };

	float strafeMinSec{ 0.7f };
	float strafeMaxSec{ 1.5f };
	int strafeChangeChancePercent{ 50 };
	bool allowNavPathing{ true };
	bool lockFacingToTarget{ false };
};

struct AIReactionRuleDef
{
	AIReactionRuleEvent event{ AIReactionRuleEvent::OnHitReceived };
	uint8_t phaseMin{ 1 };
	uint8_t phaseMax{ 255 };
	float selfHpRatioMin{ 0.0f };
	float selfHpRatioMax{ 1.0f };
	AIReactionRuleOutcome outcome{ AIReactionRuleOutcome::Ignore };
	bool retargetAttacker{ false };
	AbilityId reactAbilityId{ InvalidAbilityId };
	float reactDurationSec{ 0.0f };
	int priority{ 0 };
};

struct AIBehaviorProfileDef
{
	AIBehaviorProfileId id{ InvalidAIBehaviorProfileId };
	std::string key;
	AIArchetype aiType{ AIArchetype::None };

	AIPerceptionTuningDef perception{};
	AITargetingTuningDef targeting{};
	AIDecisionTuningDef decision{};

	std::span<const AIActionDef> combatActionDefs{};
	std::span<const AIActionDef> idleActionDefs{};
	std::span<const AIMovementProfileDef> movementProfiles{};
	std::span<const AIReactionRuleDef> reactionRules{};
	std::span<const AIPhaseTransitionDef> phaseTransitions{};
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
	std::vector<std::vector<AIActionDef>> combatActionDefStorage;
	std::vector<std::vector<AIActionDef>> idleActionDefStorage;
	std::vector<std::vector<AIMovementProfileDef>> movementProfileStorage;
	std::vector<std::vector<AIReactionRuleDef>> reactionRuleStorage;
	std::vector<std::vector<AIPhaseTransitionDef>> phaseTransitionStorage;

	void Clear() noexcept
	{
		profiles.Clear();
		combatActionDefStorage.clear();
		idleActionDefStorage.clear();
		movementProfileStorage.clear();
		reactionRuleStorage.clear();
		phaseTransitionStorage.clear();
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
