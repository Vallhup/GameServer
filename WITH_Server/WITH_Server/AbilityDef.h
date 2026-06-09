#pragma once

#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "GameplayContentIds.h"
#include "GameplayEffectDef.h"
#include "GameplayTagDef.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

enum class AbilityKind : uint8_t
{
	None,
	Attack,
	Dodge,
	Parry,
	Guard,
	Reaction,
	UseItem,
	NonCombat,
	Dead
};

enum class AbilityTimelinePolicy : uint8_t
{
	FixedDuration,
	Holdable
};

enum class AbilityCostConsumeTiming : uint8_t
{
	OnRequest,
	OnCommit,
	OnWindowEnter
};

struct AbilityCostDef
{
	AttributeId attributeId{ InvalidAttributeId };
	AbilityCostConsumeTiming consumeTiming{
		AbilityCostConsumeTiming::OnRequest };
	float amount{ 0.0f };
};

struct AbilityActivationDef
{
	GameplayTagQueryDef requiredOwnerTags;
	GameplayTagQueryDef blockedOwnerTags;
	bool requiresTarget{ false };
	bool requiresGrounded{ false };
};

enum class AbilityTransitionCause : uint8_t
{
	None,
	OnHitReceived,
	OnParried,
	OnAttributeZero,
	HoldRelease,
	ManualCancel,
	Combo,
	LightAttackCancel,
	HeavyAttackCancel,
	DodgeCancel,
	ParryCancel
};

enum class AbilityTransitionWindowPolicy : uint8_t
{
	Always,
	Range
};

enum class AbilityEndType : uint8_t
{
	NaturalEnd,
	HoldRelease,
	ImmediateTransition
};

struct AbilityEndPolicyDef
{
	AbilityEndType endType{ AbilityEndType::NaturalEnd };
	AbilityId defaultNextAbilityId{ InvalidAbilityId };
};

struct AbilityTransitionRuleDef
{
	AbilityTransitionCause cause{ AbilityTransitionCause::None };
	AbilityId toAbilityId{ InvalidAbilityId };
	AbilityTransitionWindowPolicy windowPolicy{
		AbilityTransitionWindowPolicy::Always };
	std::optional<float> windowStartNormalized;
	std::optional<float> windowEndNormalized;
	int priority{ 0 };
	bool aiInterruptible{ false };
};

struct AbilityTransitionDef
{
	AbilityEndPolicyDef endPolicy;
	AbilityId defaultNextAbilityId{ InvalidAbilityId };
	std::vector<AbilityTransitionRuleDef> interruptRules;
	std::vector<AbilityTransitionRuleDef> cancelRules;
};

enum class AbilityMovementMode : uint8_t
{
	None,
	ForwardFixedDistance,
	InputDirectionDistance,
	DashToTarget
};

enum class AbilityRotationMode : uint8_t
{
	None,
	FaceMoveDirection,
	FaceTarget
};

enum class AbilityVerticalMovementMode : uint8_t
{
	None,
	FixedOffset
};

enum class AbilityDirectionPolicy : uint8_t
{
	ActionStartInput,
	CurrentInput,
	FacingDirection,
	TargetDirection,
	LockedDirection
};

enum class AbilityDirectionSampleTiming : uint8_t
{
	OnSegmentStart,
	Continuous
};

struct AbilityMovementSegmentDef
{
	float startNormalized{ 0.0f };
	float endNormalized{ 0.0f };

	AbilityMovementMode movementMode{ AbilityMovementMode::None };
	std::optional<float> moveDistance;

	AbilityRotationMode rotationMode{ AbilityRotationMode::None };
	std::optional<float> rotationRate;

	AbilityVerticalMovementMode verticalMovementMode{
		AbilityVerticalMovementMode::None };
	std::optional<float> verticalAmount;

	AbilityDirectionPolicy directionPolicy{
		AbilityDirectionPolicy::ActionStartInput };
	AbilityDirectionSampleTiming directionSampleTiming{
		AbilityDirectionSampleTiming::OnSegmentStart };
};

enum class AbilityCombatWindowKind : uint8_t
{
	None,
	Attack,
	Parry,
	Guard,
	Armor,
	Invulnerability
};

enum class AbilityCombatApplyTo : uint8_t
{
	FrontPhysical,
	ParryableAttack,
	GuardableAttack
};

enum class AbilityCombatReferenceFrame : uint8_t
{
	OwnerFacing,
	MoveDirection,
	LockedActionDirection
};

struct AbilityCombatSpatialFilterDef
{
	std::optional<float> facingHalfAngleDeg;
	std::optional<float> minDistance;
	std::optional<float> maxDistance;
	std::optional<float> verticalTolerance;
	AbilityCombatReferenceFrame referenceFrame{
		AbilityCombatReferenceFrame::OwnerFacing };
};

enum class AbilityCombatEffectType : uint8_t
{
	None,
	AttackHit,
	AreaHit,
	ParryResponse,
	GuardResponse
};

struct AbilityAttackHitDef
{
	float damageScale{ 0.0f };
	float bonusDamage{ 0.0f };
	float staminaDamageScale{ 0.0f };
	float bonusStaminaDamage{ 0.0f };
	float poiseDamageScale{ 0.0f };
	float bonusPoiseDamage{ 0.0f };
	float knockbackDistance{ 0.0f };
	float hitStopSec{ 0.0f };
	bool parryable{ false };
	bool guardable{ false };
};

struct AbilityParryResponseDef
{
	float stunSec{ 0.0f };
	float hitStopSec{ 0.0f };
	std::optional<GameplayEffectId> grantEffectId;
};

struct AbilityGuardResponseDef
{
	// Reduction is the blocked fraction; chip damage is the minimum fraction
	// of incoming HP damage that still passes through a successful guard.
	float damageReductionRatio{ 0.0f };
	float chipDamageRatio{ 0.0f };
	float staminaDamageMultiplier{ 0.0f };
	float hitStopSec{ 0.0f };
};

struct AbilityCombatEffectDef
{
	AbilityCombatEffectType type{ AbilityCombatEffectType::None };
	std::optional<AbilityAttackHitDef> attackHit;
	std::optional<AbilityParryResponseDef> parryResponse;
	std::optional<AbilityGuardResponseDef> guardResponse;
};

struct AbilityCombatWindowDef
{
	AbilityCombatWindowKind kind{ AbilityCombatWindowKind::None };
	float startNormalized{ 0.0f };
	float endNormalized{ 0.0f };
	std::optional<GameplayEffectId> applyEffectId;
	std::optional<AbilityCombatApplyTo> appliesTo;
	std::optional<AbilityCombatSpatialFilterDef> spatialFilter;
	std::vector<uint16_t> sourceHitBones;
	std::optional<AbilityCombatEffectDef> effect;
};

enum class AbilityEventKind : uint8_t
{
	None,
	ConsumeItem,
	ApplyGameplayEffect,
	SpawnProjectile,
	TriggerAreaHit,
	SpawnAreaVolume,
	PlayCue
};

enum class AbilityEventTriggerCondition : uint8_t
{
	Always,
	OnParrySuccess
};

struct AbilityEventDef
{
	AbilityEventKind kind{ AbilityEventKind::None };
	float timeNormalized{ 0.0f };
	std::optional<GameplayEffectId> effectId;
	std::optional<GameplayTagId> cueTagId;
	std::optional<uint16_t> payloadId;
	std::optional<ProjectileId> projectileId;
	std::optional<AreaHitId> areaHitId;
	std::optional<std::string> projectileKey;
	std::optional<std::string> areaHitKey;
	AbilityEventTriggerCondition condition{
		AbilityEventTriggerCondition::Always };
};

struct AbilityTimelineDef
{
	float durationSec{ 0.0f };
	AbilityTimelinePolicy policy{ AbilityTimelinePolicy::FixedDuration };

	std::vector<AbilityCombatWindowDef> combatWindows;
	std::vector<AbilityMovementSegmentDef> movementSegments;
	std::vector<AbilityEventDef> events;
};

struct AbilityDef
{
	AbilityId id{ InvalidAbilityId };
	std::string key;
	std::string name;
	AbilityKind kind{ AbilityKind::None };
	GameplayTagMask tags{ 0 };

	AbilityActivationDef activation;
	std::vector<AbilityCostDef> costs;
	AbilityTimelineDef timeline;
	AbilityTransitionDef transition;
};

struct AbilityDefTraits
{
	static AbilityId GetId(const AbilityDef& def) noexcept
	{
		return def.id;
	}
};

using AbilityDefRegistry = DefRegistry<
	AbilityDef,
	AbilityId,
	AbilityDefTraits>;

bool ValidateAbilityDefs(
	std::span<const AbilityDef> defs,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayEffectDef> effects,
	std::span<const GameplayTagDef> tags,
	std::string& outError);

DefLoadResult LoadAbilityDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayEffectDef> effects,
	std::span<const GameplayTagDef> tags,
	AbilityDefRegistry& outRegistry);
