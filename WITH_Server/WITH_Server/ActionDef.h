#pragma once

#include "AnimationId.h"
#include "IDs.h"

#include <optional>
#include <string>
#include <span>
#include <vector>

enum class ActionKind : uint8_t
{
	None,
	Attack, 
	Dodge,
	Parry,
	Stun,
	Hit,
	Guard,
	UseItem,
	Dead
};

enum class PlayerActionInput : uint8_t
{
	None,
	LightAttack,
	HeavAttack,
	Guard,
	Parry,
	Dodge,
	UseItem
};

enum class ActionNormalizedPolicy : uint8_t
{
	FixedDuration,
	Holdable
};

enum class ActionEndType : uint8_t
{
	NaturalEnd,
	HoldRelease,
	ImmediateTransition
};

struct ActionEndPolicyDef
{
	ActionEndType endType;
	ActionId defaultNextActionId;
};

enum class ActionRequestRequirementType : uint8_t
{
	None,
	HasEnoughStamina,
	MissingStateFlag,
	HasStateFlag,
	HasTarget,
	IsGrounded
};

struct ActionRequestRequirementDef
{
	ActionRequestRequirementType type;
	std::optional<float> scalar;
	std::optional<GameplayStateFlag> stateFlag;
};

enum class ActionResourceType : uint8_t
{
	Hp,
	Stamina
};

enum class ActionResourceConsumeTiming : uint8_t
{
	OnRequest,
	OnCommit,
	OnWindowEnter
};

struct ActionResourceCostDef
{
	ActionResourceType type;
	ActionResourceConsumeTiming consumeTiming;
	float amount;
};

enum class ActionInterruptCauseType : uint8_t
{
	OnHitReceived,
	OnParried,
	OnHpZero
};

enum class ActionWindowPolicy : uint8_t
{
	Always,
	Range
};

struct ActionInterruptRule
{
	ActionInterruptCauseType causeType;
	ActionId toActionId;
	ActionWindowPolicy windowPolicy;
	std::optional<float> windowStartNormalized;
	std::optional<float> windowEndNormalized;
	int priority;
};

enum class ActionCancelKind : uint8_t
{
	Combo,
	HoldRelease,
	LightAttackCancel,
	HeavyAttackCancel,
	DodgeCancel,
	ParryCancel
};

struct ActionCancelRule
{
	ActionCancelKind cancelKind;
	ActionId toActionId;
	ActionWindowPolicy windowPolicy;
	std::optional<float> windowStartNormalized;
	std::optional<float> windowEndNormalized;
	int priority;
	// true이면 AI Decision System이 이 cancel window에서 새 action 발행 가능
	bool aiInterruptible{ false };
};

struct ActionTransitionRuleDef
{
	std::vector<ActionInterruptRule> interruptRules;
	std::vector<ActionCancelRule> cancelRules;
};

enum class HorizontalMovementMode : uint8_t
{
	None,
	ForwardFixedDistance,
	InputDirectionDistance
};

enum class RotationMode : uint8_t
{
	None,
	FaceMoveDirection,
	FaceTarget
};

enum class VerticalMovementMode : uint8_t
{
	None,
	FixedOffset
};

enum class DirectionPolicy : uint8_t
{
	ActionStartInput,
	CurrentInput,
	FacingDirection,
	TargetDirection,
	LockedDirection
};

enum class DirectionSampleTiming : uint8_t
{
	OnSegmentStart,
	Continuous
};

struct ActionMovementSegmentDef
{
	float startNormalized;
	float endNormalized;

	HorizontalMovementMode horizontalMoveMode;
	std::optional<float> moveDistance;

	RotationMode rotationMode;
	std::optional<float> rotationRate;

	VerticalMovementMode verticalMoveMode;
	std::optional<float> verticalAmount;

	DirectionPolicy dirPolicy;
	DirectionSampleTiming dirSampleTiming;
};

enum class CombatWindowType : uint8_t
{
	Attack,
	Parry,
	Guard,
	Armor,
	Invulnerability
};

enum class ActionCombatApplyTo : uint8_t
{
	FrontPhysical,
	ParryableAttack,
	GuardableAttack
};

enum class CombatReferenceFrame : uint8_t
{
	OwnerFacing,
	MoveDirection,
	LockedActionDirection
};

struct ActionCombatSpatialFilterDef
{
	std::optional<float> facingHalfAngleDeg;
	std::optional<float> minDistance;
	std::optional<float> maxDistance;
	std::optional<float> verticalTolerance;

	CombatReferenceFrame referenceFrame = CombatReferenceFrame::OwnerFacing;
};

enum class CombatEffectType : uint8_t
{
	None,
	AttackHit,
	ParryResponse,
	GuardResponse
};

struct AttackCombatEffectDef
{
	float damageScale;
	float bonusDamage;

	float staminaDamageScale;
	float bonusStaminaDamage;

	float poiseDamageScale;
	float bonusPoiseDamage;

	float knockbackDistance;
	float hitStopSec;

	bool parryable;
	bool guardable;
};

struct ParryCombatEffectDef
{
	float stunSec;
	float hitStopSec;

	std::optional<BuffId> grantBuffId;
};

struct GuardCombatEffectDef
{
	float damageReductionRatio;
	float chipDamageRatio;
	float staminaDamageMultiplier;
	float hitStopSec;
};

struct CombatEffectDef
{
	CombatEffectType type;

	std::optional<AttackCombatEffectDef> attackHit;
	std::optional<ParryCombatEffectDef> parryResponse;
	std::optional<GuardCombatEffectDef> guardResponse;
};

struct ActionCombatWindowDef
{
	CombatWindowType windowType;
	float startNormalized;
	float endNormalized;

	std::optional<ActionCombatApplyTo> appliesTo;
	std::optional<ActionCombatSpatialFilterDef> spatialFilter;
	std::optional<CombatEffectDef> effect;
};

enum class EventType : uint8_t
{
	ConsumeItem,
	ApplyGameplayEffect,
	SpawnProjectile,
	PlayEffect
};

enum class TriggerConditionType : uint8_t
{
	Always,
	OnParrySuccess
};

using EventPayloadId = uint16_t;

struct ActionEventDef
{
	EventType type;
	float timeNormalized;
	std::optional<EventPayloadId> payloadId;
	TriggerConditionType conditionType;
};

using ComboGroupId = uint16_t;

struct ActionDef
{
	ActionId id;
	std::string name;
	
	CharacterId characterId;
	ActionKind kind;
	PlayerActionInput playerInput;

	std::optional<ComboGroupId> comboGroupId;
	std::optional<uint8_t> comboIndex;

	float duration;
	ActionNormalizedPolicy normalizedPolicy;

	ActionEndPolicyDef endPolicy;
	std::vector<ActionRequestRequirementDef> requestRequirements;
	std::vector<ActionResourceCostDef> resourceCosts;
	ActionTransitionRuleDef transitionRule;
	std::vector<ActionCombatWindowDef> combatWindows;
	std::vector<ActionEventDef> events;
	std::vector<ActionMovementSegmentDef> moveSegments;
};

const ActionDef* FindActionDef(ActionId id) noexcept;
const ActionDef& GetActionDef(ActionId id);
std::span<const ActionDef> GetActionDefs() noexcept;
