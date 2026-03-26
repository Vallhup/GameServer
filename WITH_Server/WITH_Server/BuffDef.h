#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include "IDs.h"

enum class BuffKind : uint8_t
{
	None,
	Positive,
	Negative,
	Control
};

using StackGroupId = uint16_t;

struct BuffProfileDef
{
	BuffId id;
	std::string name;

	BuffKind kind;
	StackGroupId groupId;
};

enum class DurationPolicy : uint8_t
{
	Instant,
	Timed,
	Infinite
};

enum class OverlappingPolicy : uint8_t
{
	Reject,
	Refresh,
	Stack
};

enum class ReapplyPolicy : uint8_t
{
	None,
	RefreshDuration,
	RefreshStacks,
	RefreshDurationAndStacks
};

struct BuffLifetimeDef
{
	DurationPolicy durationPolicy;
	float defaultDurationSec;
	
	std::optional<float> tickIntervalSec;

	uint8_t maxStackCount;

	OverlappingPolicy overlappingPolicy;
	ReapplyPolicy reapplyPolicy;
};

enum class BuffModifierKind : uint8_t
{
	None,
	StatAdd,
	StatMul,
	StateFlag,
	PeriodicEffect
};

enum class BuffModifierTiming : uint8_t
{
	Always,
	OnTick
};

enum class PeriodicEffectType : uint8_t
{
	DamageHp,
	RecoverHp,
	RecoverStamina
};

enum class BuffModifierConditionType : uint8_t
{
	None,
	HasStateFlag,
	MissingStateFlag
};

struct BuffModifierCondition
{
	BuffModifierConditionType type;
	StateFlagType stateFlagType;
};

struct BuffModifierDef
{
	BuffModifierKind kind;
	BuffModifierTiming timing;

	// Kind = StatAdd / StatMul
	float value;
	StatType statType;

	// Kind = StateFlag
	StateFlagType stateFlagType;

	// Kind = PeriodicEffect
	PeriodicEffectType periodicEffectType;

	std::optional<BuffModifierCondition> condition;
};

enum class BuffApplyRequirementType : uint8_t
{
	None,
	AlreadyApplied,
	NotAlreadyApplied,
	HasStateFlag,
	MissingStateFlag,
	HpRatioAbove,
	HpRatioBelow,
	TargetFactionIsEnemy
};

struct BuffApplyRequirementDef
{
	BuffApplyRequirementType type;
	float parameter;
};

enum class BuffRemoveRuleType : uint8_t
{
	None,
	OnDurationExpired,
	OnOwnerDamaged,
	OnOwnerActionCommitted,
	OnStackDepleted,
	OnStateFlagMissing
};

struct BuffRemoveRuleDef
{
	BuffRemoveRuleType type;
	float parameter;
};

struct BuffDef
{
	BuffProfileDef profile;
	BuffLifetimeDef lifetime;

	std::vector<BuffModifierDef> modifiers;
	std::vector<BuffApplyRequirementDef> applyRequirements;
	std::vector<BuffRemoveRuleDef> removeRules;
};