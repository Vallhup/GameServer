#pragma once

#include "AttributeDef.h"
#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "GameplayContentIds.h"
#include "GameplayTagDef.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

enum class GameplayEffectKind : uint8_t
{
	None,
	Positive,
	Negative,
	Control
};

enum class GameplayEffectDurationPolicy : uint8_t
{
	Instant,
	Timed,
	Infinite
};

struct GameplayEffectLifetimeDef
{
	GameplayEffectDurationPolicy durationPolicy{
		GameplayEffectDurationPolicy::Instant };
	float defaultDurationSec{ 0.0f };
	std::optional<float> tickIntervalSec;
};

enum class GameplayEffectStackPolicy : uint8_t
{
	Independent,
	Refresh,
	Stack,
	Replace
};

struct GameplayEffectStackingDef
{
	std::string groupKey;
	uint8_t maxStackCount{ 1 };
	GameplayEffectStackPolicy stackPolicy{
		GameplayEffectStackPolicy::Independent };
};

enum class AttributeModifierOp : uint8_t
{
	Add,
	Multiply
};

struct AttributeModifierDef
{
	AttributeId attributeId{ InvalidAttributeId };
	AttributeModifierOp op{ AttributeModifierOp::Add };
	float value{ 0.0f };
};

enum class GameplayTagModifierOp : uint8_t
{
	Add,
	Remove
};

struct GameplayTagModifierDef
{
	GameplayTagId tagId{ InvalidGameplayTagId };
	GameplayTagModifierOp op{ GameplayTagModifierOp::Add };
};

enum class PeriodicEffectKind : uint8_t
{
	None,
	AttributeDelta
};

struct PeriodicEffectDef
{
	PeriodicEffectKind kind{ PeriodicEffectKind::None };
	AttributeId attributeId{ InvalidAttributeId };
	float value{ 0.0f };
};

struct GameplayEffectRequirementDef
{
	GameplayTagQueryDef requiredTags;
	GameplayTagQueryDef blockedTags;
};

enum class GameplayEffectRemoveRuleTrigger : uint8_t
{
	TagQueryMatched,
	CounterReached,
	DurationExpired
};

enum class GameplayEffectCounterEvent : uint8_t
{
	None,
	AbilityCommitted
};

struct GameplayEffectRemoveRuleDef
{
	GameplayEffectRemoveRuleTrigger trigger{
		GameplayEffectRemoveRuleTrigger::TagQueryMatched };
	GameplayTagQueryDef removeWhenTagsMatch;
	std::optional<float> counterThreshold;
	std::optional<std::string> abilityKind;
	GameplayEffectCounterEvent counterEvent{
		GameplayEffectCounterEvent::None };
};

struct GameplayEffectDef
{
	GameplayEffectId id{ InvalidGameplayEffectId };
	std::string key;
	std::string name;
	GameplayEffectKind kind{ GameplayEffectKind::None };
	GameplayTagMask tags{ 0 };

	GameplayEffectLifetimeDef lifetime;
	GameplayEffectStackingDef stacking;

	std::vector<AttributeModifierDef> attributeModifiers;
	std::vector<GameplayTagModifierDef> tagModifiers;
	std::vector<PeriodicEffectDef> periodicEffects;

	std::vector<GameplayEffectRequirementDef> applyRequirements;
	std::vector<GameplayEffectRemoveRuleDef> removeRules;
};

struct GameplayEffectDefTraits
{
	static GameplayEffectId GetId(const GameplayEffectDef& def) noexcept
	{
		return def.id;
	}
};

using GameplayEffectDefRegistry = DefRegistry<
	GameplayEffectDef,
	GameplayEffectId,
	GameplayEffectDefTraits>;

bool ValidateGameplayEffectDefs(
	std::span<const GameplayEffectDef> defs,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayTagDef> tags,
	std::string& outError);

DefLoadResult LoadGameplayEffectDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayTagDef> tags,
	GameplayEffectDefRegistry& outRegistry);
