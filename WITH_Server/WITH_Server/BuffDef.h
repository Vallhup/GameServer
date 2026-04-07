#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <span>
#include <vector>
#include "IDs.h"

enum class ActionKind : uint8_t;

enum class BuffKind : uint8_t
{
	None,
	Positive,
	Negative,
	Control
};

enum class BuffFamilyId : uint16_t
{
	None = 0,

	HpBoost,
	StaminaBoost,
	AttackBoost,
	AttackSpeedBoost,
	DefenseBoost,
	MoveSpeedBoost,

	ParrySuccess
};

enum class BuffTier : uint8_t
{
	None = 0,
	Low = 1,
	Mid = 2,
	High = 3
};

using StackGroupId = BuffFamilyId;

enum class BuffFamilyStackPolicy : uint8_t
{
	Independent,
	ReplaceWithHigherTier
};

struct BuffProfileDef
{
	BuffId id;
	std::string name;

	BuffKind kind;
	BuffFamilyId familyId;
	BuffTier tier;
	StackGroupId groupId;
	BuffFamilyStackPolicy familyStackPolicy;
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

enum class BuffEffectType : uint8_t
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
	GameplayStateFlag stateFlag;
};

struct BuffStatEffectDef
{
	StatType statType;
	float value;
};

struct BuffStateFlagEffectDef
{
	GameplayStateFlag flag;
};

struct BuffPeriodicEffectDef
{
	PeriodicEffectType type;
	float value;
};

struct BuffEffectDef
{
	BuffEffectType type;
	BuffModifierTiming timing;

	std::optional<BuffStatEffectDef> stat;
	std::optional<BuffStateFlagEffectDef> stateFlag;
	std::optional<BuffPeriodicEffectDef> periodic;

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
	TargetFactionIs
};

struct BuffApplyRequirementOperand
{
	std::optional<float> scalarCondition;
	std::optional<GameplayStateFlag> stateFlagCondition;
	std::optional<BuffFamilyId> buffFamilyIdCondition;
	std::optional<Faction> factionCondition;
};

struct BuffApplyRequirementDef
{
	BuffApplyRequirementType type;
	BuffApplyRequirementOperand operand;
};

enum class BuffCounterEventType : uint8_t
{
	None,
	OwnerDeath,
	OwnerKill,
	OwnerDamaged,
	ActionCommitted,
	ParrySuccess,
	GuardSuccess
};

enum class BuffRemoveRuleType : uint8_t
{
	None,
	OnDurationExpired,
	OnOwnerDamaged,
	OnOwnerActionCommitted,
	OnSpecificActionCommitted,
	OnStackDepleted,
	OnStateFlagMissing,
	OnBuffFamilyApplied,
	OnCounterReached
};

struct BuffRemoveRuleOperand
{
	std::optional<float> scalarCondition;
	std::optional<GameplayStateFlag> stateFlagCondition;
	std::optional<ActionId> actionIdCondition;
	std::optional<ActionKind> actionKindCondition;
	std::optional<BuffFamilyId> buffFamilyIdCondition;
	std::optional<BuffCounterEventType> counterEventCondition;
};

struct BuffRemoveRuleDef
{
	BuffRemoveRuleType type;
	BuffRemoveRuleOperand operand;
};

struct BuffDef
{
	BuffProfileDef profile;
	BuffLifetimeDef lifetime;

	std::vector<BuffEffectDef> effects;
	std::vector<BuffApplyRequirementDef> applyRequirements;
	std::vector<BuffRemoveRuleDef> removeRules;
};

const BuffDef* FindBuffDef(BuffId id) noexcept;
const BuffDef& GetBuffDef(BuffId id);
std::span<const BuffDef> GetBuffDefs() noexcept;
