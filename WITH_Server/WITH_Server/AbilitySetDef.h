#pragma once

#include "AbilityDef.h"
#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "GameplayContentIds.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

enum class LocomotionMode : uint8_t;

enum class AbilityRequestSemantic : uint8_t
{
	None,
	LightAttack,
	HeavyAttack,
	Dodge,
	Parry,
	GuardStart,
	UseItem
};

enum class AbilityCandidateSelectionPolicy : uint8_t
{
	OrderedFirstValid,
	HighestPriorityValid
};

struct AbilityGrantDef
{
	AbilityId abilityId{ InvalidAbilityId };
	uint16_t level{ 1 };
};

struct AbilityInputBindingEntryDef
{
	AbilityRequestSemantic request{ AbilityRequestSemantic::None };
	std::vector<AbilityId> candidateAbilities;
	AbilityCandidateSelectionPolicy selectionPolicy{
		AbilityCandidateSelectionPolicy::OrderedFirstValid };
	int priority{ 0 };
};

struct AbilityInputBindingProfileDef
{
	AbilityInputBindingProfileId id{ InvalidAbilityInputBindingProfileId };
	std::string key;
	std::vector<AbilityInputBindingEntryDef> entries;
};

struct AbilityAnimationBindingDef
{
	AbilityId abilityId{ InvalidAbilityId };
	std::string animationKey;
	float playRate{ 1.0f };
	float startNormalizedTime{ 0.0f };
};

struct AbilityLocomotionAnimationBindingDef
{
	LocomotionMode mode;
	std::string animationKey;
	float playRate{ 1.0f };
	bool holdLastFrame{ false };
};

struct AbilityAnimationBindingProfileDef
{
	AbilityAnimationBindingProfileId id{
		InvalidAbilityAnimationBindingProfileId };
	std::string key;
	std::vector<AbilityAnimationBindingDef> abilityBindings;
	std::vector<AbilityLocomotionAnimationBindingDef> locomotionBindings;
};

struct AbilityFallbackReactionEntryDef
{
	AbilityTransitionCause cause{ AbilityTransitionCause::OnHitReceived };
	AbilityId toAbilityId{ InvalidAbilityId };
	int priority{ 0 };
};

struct AbilityFallbackReactionProfileDef
{
	AbilityFallbackReactionProfileId id{
		InvalidAbilityFallbackReactionProfileId };
	std::string key;
	std::vector<AbilityFallbackReactionEntryDef> entries;
};

struct AbilitySetDef
{
	AbilitySetId id{ InvalidAbilitySetId };
	std::string key;
	std::string name;
	std::vector<AbilityGrantDef> grants;

	std::optional<AbilityInputBindingProfileId> inputBindingProfileId;
	std::optional<AbilityAnimationBindingProfileId> animationBindingProfileId;
	std::optional<AbilityFallbackReactionProfileId> fallbackReactionProfileId;
};

struct AbilitySetDefTraits
{
	static AbilitySetId GetId(const AbilitySetDef& def) noexcept
	{
		return def.id;
	}
};

struct AbilityInputBindingProfileDefTraits
{
	static AbilityInputBindingProfileId GetId(
		const AbilityInputBindingProfileDef& def) noexcept
	{
		return def.id;
	}
};

struct AbilityAnimationBindingProfileDefTraits
{
	static AbilityAnimationBindingProfileId GetId(
		const AbilityAnimationBindingProfileDef& def) noexcept
	{
		return def.id;
	}
};

struct AbilityFallbackReactionProfileDefTraits
{
	static AbilityFallbackReactionProfileId GetId(
		const AbilityFallbackReactionProfileDef& def) noexcept
	{
		return def.id;
	}
};

using AbilitySetDefRegistry = DefRegistry<
	AbilitySetDef,
	AbilitySetId,
	AbilitySetDefTraits>;
using AbilityInputBindingProfileDefRegistry = DefRegistry<
	AbilityInputBindingProfileDef,
	AbilityInputBindingProfileId,
	AbilityInputBindingProfileDefTraits>;
using AbilityAnimationBindingProfileDefRegistry = DefRegistry<
	AbilityAnimationBindingProfileDef,
	AbilityAnimationBindingProfileId,
	AbilityAnimationBindingProfileDefTraits>;
using AbilityFallbackReactionProfileDefRegistry = DefRegistry<
	AbilityFallbackReactionProfileDef,
	AbilityFallbackReactionProfileId,
	AbilityFallbackReactionProfileDefTraits>;

struct AbilitySetDefinitionRegistries
{
	AbilitySetDefRegistry abilitySets;
	AbilityInputBindingProfileDefRegistry inputBindingProfiles;
	AbilityAnimationBindingProfileDefRegistry animationBindingProfiles;
	AbilityFallbackReactionProfileDefRegistry fallbackReactionProfiles;
};

bool ValidateAbilitySetDefs(
	std::span<const AbilitySetDef> sets,
	std::span<const AbilityDef> abilities,
	std::span<const AbilityInputBindingProfileDef> inputProfiles,
	std::span<const AbilityAnimationBindingProfileDef> animationProfiles,
	std::span<const AbilityFallbackReactionProfileDef> fallbackProfiles,
	std::string& outError);

DefLoadResult LoadAbilitySetDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AbilityDef> abilities,
	AbilitySetDefinitionRegistries& outRegistries);
