#include "pch.h"
#include "AbilitySetDef.h"
#include "DefEnumString.h"
#include "GameplayEffectDef.h"
#include "AnimationId.h"

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>

namespace
{
	template<typename TId>
	struct IdHash
	{
		size_t operator()(TId id) const noexcept
		{
			return std::hash<TId>{}(id);
		}
	};

	template<typename TId>
	bool ContainsId(const std::unordered_set<TId, IdHash<TId>>& ids, TId id)
	{
		return ids.find(id) != ids.end();
	}

	bool ValidateTagQuery(
		const GameplayTagQueryDef& query,
		const std::unordered_set<GameplayTagId, IdHash<GameplayTagId>>& tagIds,
		std::string& outError)
	{
		for (const GameplayTagId tagId : query.tags)
		{
			if (tagId == InvalidGameplayTagId || !ContainsId(tagIds, tagId))
			{
				outError = "Gameplay tag query references unknown tag.";
				return false;
			}
		}

		for (const GameplayTagQueryDef& child : query.children)
		{
			if (!ValidateTagQuery(child, tagIds, outError))
				return false;
		}

		return true;
	}

	bool IsNormalizedRange(float start, float end) noexcept
	{
		return start >= 0.0f && end <= 1.0f && start <= end;
	}
}

bool ValidateGameplayTagDefs(
	std::span<const GameplayTagDef> defs,
	std::string& outError)
{
	std::unordered_set<GameplayTagId, IdHash<GameplayTagId>> ids;
	ids.reserve(defs.size());

	for (const GameplayTagDef& def : defs)
	{
		if (def.id == InvalidGameplayTagId)
		{
			outError = "Gameplay tag id must not be invalid.";
			return false;
		}

		if (def.key.empty() || def.name.empty() || def.path.empty())
		{
			outError = "Gameplay tag key, name, and path must not be empty.";
			return false;
		}

		if (!ids.insert(def.id).second)
		{
			outError = "Duplicate gameplay tag id detected.";
			return false;
		}
	}

	for (const GameplayTagDef& def : defs)
	{
		for (const GameplayTagId parentId : def.parents)
		{
			if (parentId == InvalidGameplayTagId || !ContainsId(ids, parentId))
			{
				outError = "Gameplay tag references unknown parent tag.";
				return false;
			}
		}
	}

	return true;
}

bool ValidateAttributeDefs(
	std::span<const AttributeDef> defs,
	std::string& outError)
{
	std::unordered_set<AttributeId, IdHash<AttributeId>> ids;
	ids.reserve(defs.size());

	for (const AttributeDef& def : defs)
	{
		if (def.id == InvalidAttributeId)
		{
			outError = "Attribute id must not be invalid.";
			return false;
		}

		if (def.key.empty() || def.name.empty())
		{
			outError = "Attribute key and name must not be empty.";
			return false;
		}

		if (def.clampToRange && def.minValue > def.maxValue)
		{
			outError = "Attribute range is invalid.";
			return false;
		}

		if (!ids.insert(def.id).second)
		{
			outError = "Duplicate attribute id detected.";
			return false;
		}
	}

	return true;
}

bool ValidateGameplayEffectDefs(
	std::span<const GameplayEffectDef> defs,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayTagDef> tags,
	std::string& outError)
{
	std::unordered_set<GameplayEffectId, IdHash<GameplayEffectId>> effectIds;
	std::unordered_set<AttributeId, IdHash<AttributeId>> attributeIds;
	std::unordered_set<GameplayTagId, IdHash<GameplayTagId>> tagIds;

	effectIds.reserve(defs.size());
	attributeIds.reserve(attributes.size());
	tagIds.reserve(tags.size());

	for (const AttributeDef& attribute : attributes)
		attributeIds.insert(attribute.id);

	for (const GameplayTagDef& tag : tags)
		tagIds.insert(tag.id);

	for (const GameplayEffectDef& def : defs)
	{
		if (def.id == InvalidGameplayEffectId)
		{
			outError = "Gameplay effect id must not be invalid.";
			return false;
		}

		if (def.key.empty() || def.name.empty())
		{
			outError = "Gameplay effect key and name must not be empty.";
			return false;
		}

		if (def.kind == GameplayEffectKind::None)
		{
			outError = "Gameplay effect kind must not be None.";
			return false;
		}

		if (def.lifetime.durationPolicy == GameplayEffectDurationPolicy::Timed &&
			def.lifetime.defaultDurationSec <= 0.0f)
		{
			outError = "Timed gameplay effect requires positive duration.";
			return false;
		}

		if (def.stacking.maxStackCount == 0)
		{
			outError = "Gameplay effect max stack count must be positive.";
			return false;
		}

		if (def.stacking.groupKey.empty())
		{
			outError = "Gameplay effect stacking group key must not be empty.";
			return false;
		}

		if (!effectIds.insert(def.id).second)
		{
			outError = "Duplicate gameplay effect id detected.";
			return false;
		}

		for (const AttributeModifierDef& modifier : def.attributeModifiers)
		{
			if (modifier.attributeId == InvalidAttributeId ||
				!ContainsId(attributeIds, modifier.attributeId))
			{
				outError = "Gameplay effect references unknown attribute.";
				return false;
			}
		}

		for (const GameplayTagModifierDef& modifier : def.tagModifiers)
		{
			if (modifier.tagId == InvalidGameplayTagId ||
				!ContainsId(tagIds, modifier.tagId))
			{
				outError = "Gameplay effect references unknown gameplay tag.";
				return false;
			}
		}

		for (const PeriodicEffectDef& periodic : def.periodicEffects)
		{
			if (periodic.attributeId == InvalidAttributeId ||
				!ContainsId(attributeIds, periodic.attributeId))
			{
				outError = "Gameplay effect periodic effect references unknown attribute.";
				return false;
			}
		}

		for (const GameplayEffectRequirementDef& requirement :
			def.applyRequirements)
		{
			if (!ValidateTagQuery(requirement.requiredTags, tagIds, outError) ||
				!ValidateTagQuery(requirement.blockedTags, tagIds, outError))
			{
				return false;
			}
		}

		for (const GameplayEffectRemoveRuleDef& removeRule : def.removeRules)
		{
			if (!ValidateTagQuery(
				removeRule.removeWhenTagsMatch,
				tagIds,
				outError))
			{
				return false;
			}
		}
	}

	return true;
}

bool ValidateAbilityDefs(
	std::span<const AbilityDef> defs,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayEffectDef> effects,
	std::span<const GameplayTagDef> tags,
	std::string& outError)
{
	std::unordered_set<AbilityId, IdHash<AbilityId>> abilityIds;
	std::unordered_set<AttributeId, IdHash<AttributeId>> attributeIds;
	std::unordered_set<GameplayEffectId, IdHash<GameplayEffectId>> effectIds;
	std::unordered_set<GameplayTagId, IdHash<GameplayTagId>> tagIds;

	abilityIds.reserve(defs.size());
	attributeIds.reserve(attributes.size());
	effectIds.reserve(effects.size());
	tagIds.reserve(tags.size());

	for (const AttributeDef& attribute : attributes)
		attributeIds.insert(attribute.id);

	for (const GameplayEffectDef& effect : effects)
		effectIds.insert(effect.id);

	for (const GameplayTagDef& tag : tags)
		tagIds.insert(tag.id);

	for (const AbilityDef& def : defs)
	{
		if (def.id == InvalidAbilityId)
		{
			outError = "Ability id must not be invalid.";
			return false;
		}

		if (def.key.empty() || def.name.empty())
		{
			outError = "Ability key and name must not be empty.";
			return false;
		}

		if (def.kind == AbilityKind::None)
		{
			outError = "Ability kind must not be None.";
			return false;
		}

		if (def.timeline.durationSec < 0.0f)
		{
			outError = "Ability duration must not be negative.";
			return false;
		}

		if (!abilityIds.insert(def.id).second)
		{
			outError = "Duplicate ability id detected.";
			return false;
		}
	}

	for (const AbilityDef& def : defs)
	{
		if (!ValidateTagQuery(
				def.activation.requiredOwnerTags,
				tagIds,
				outError) ||
			!ValidateTagQuery(
				def.activation.blockedOwnerTags,
				tagIds,
				outError))
		{
			return false;
		}

		for (const AbilityCostDef& cost : def.costs)
		{
			if (cost.attributeId == InvalidAttributeId ||
				!ContainsId(attributeIds, cost.attributeId))
			{
				outError = "Ability cost references unknown attribute.";
				return false;
			}
		}

		if (def.transition.defaultNextAbilityId != InvalidAbilityId &&
			!ContainsId(abilityIds, def.transition.defaultNextAbilityId))
		{
			outError = "Ability default transition references unknown ability.";
			return false;
		}

		if (def.transition.endPolicy.defaultNextAbilityId != InvalidAbilityId &&
			!ContainsId(
				abilityIds,
				def.transition.endPolicy.defaultNextAbilityId))
		{
			outError = "Ability end policy references unknown ability.";
			return false;
		}

		for (const AbilityTransitionRuleDef& rule :
			def.transition.interruptRules)
		{
			if (rule.toAbilityId == InvalidAbilityId ||
				!ContainsId(abilityIds, rule.toAbilityId))
			{
				outError = "Ability interrupt rule references unknown ability.";
				return false;
			}
		}

		for (const AbilityTransitionRuleDef& rule : def.transition.cancelRules)
		{
			if (rule.toAbilityId == InvalidAbilityId ||
				!ContainsId(abilityIds, rule.toAbilityId))
			{
				outError = "Ability cancel rule references unknown ability.";
				return false;
			}
		}

		for (const AbilityCombatWindowDef& window :
			def.timeline.combatWindows)
		{
			if (!IsNormalizedRange(window.startNormalized, window.endNormalized))
			{
				outError = "Ability combat window range is invalid.";
				return false;
			}

			if (window.applyEffectId.has_value() &&
				!ContainsId(effectIds, *window.applyEffectId))
			{
				outError = "Ability combat window references unknown effect.";
				return false;
			}

			if (window.effect.has_value() &&
				window.effect->parryResponse.has_value() &&
				window.effect->parryResponse->grantEffectId.has_value() &&
				!ContainsId(
					effectIds,
					*window.effect->parryResponse->grantEffectId))
			{
				outError = "Ability parry response references unknown effect.";
				return false;
			}
		}

		for (const AbilityMovementSegmentDef& segment :
			def.timeline.movementSegments)
		{
			if (!IsNormalizedRange(segment.startNormalized, segment.endNormalized))
			{
				outError = "Ability movement segment range is invalid.";
				return false;
			}
		}

		for (const AbilityEventDef& eventDef : def.timeline.events)
		{
			if (eventDef.timeNormalized < 0.0f ||
				eventDef.timeNormalized > 1.0f)
			{
				outError = "Ability event time is outside normalized range.";
				return false;
			}

			if (eventDef.effectId.has_value() &&
				!ContainsId(effectIds, *eventDef.effectId))
			{
				outError = "Ability event references unknown effect.";
				return false;
			}

			if (eventDef.cueTagId.has_value() &&
				!ContainsId(tagIds, *eventDef.cueTagId))
			{
				outError = "Ability event references unknown cue tag.";
				return false;
			}
		}
	}

	return true;
}

bool ValidateAbilitySetDefs(
	std::span<const AbilitySetDef> sets,
	std::span<const AbilityDef> abilities,
	std::span<const AbilityInputBindingProfileDef> inputProfiles,
	std::span<const AbilityAnimationBindingProfileDef> animationProfiles,
	std::span<const AbilityFallbackReactionProfileDef> fallbackProfiles,
	std::string& outError)
{
	std::unordered_set<AbilitySetId, IdHash<AbilitySetId>> setIds;
	std::unordered_set<AbilityId, IdHash<AbilityId>> abilityIds;
	std::unordered_set<AbilityInputBindingProfileId,
		IdHash<AbilityInputBindingProfileId>> inputProfileIds;
	std::unordered_set<AbilityAnimationBindingProfileId,
		IdHash<AbilityAnimationBindingProfileId>> animationProfileIds;
	std::unordered_set<AbilityFallbackReactionProfileId,
		IdHash<AbilityFallbackReactionProfileId>> fallbackProfileIds;

	setIds.reserve(sets.size());
	abilityIds.reserve(abilities.size());
	inputProfileIds.reserve(inputProfiles.size());
	animationProfileIds.reserve(animationProfiles.size());
	fallbackProfileIds.reserve(fallbackProfiles.size());

	for (const AbilityDef& ability : abilities)
		abilityIds.insert(ability.id);

	for (const AbilityInputBindingProfileDef& profile : inputProfiles)
	{
		if (profile.id == InvalidAbilityInputBindingProfileId ||
			!inputProfileIds.insert(profile.id).second)
		{
			outError = "Invalid or duplicate ability input binding profile id.";
			return false;
		}

		if (profile.key.empty())
		{
			outError = "Ability input binding profile key must not be empty.";
			return false;
		}

		for (const AbilityInputBindingEntryDef& entry : profile.entries)
		{
			for (const AbilityId abilityId : entry.candidateAbilities)
			{
				if (abilityId == InvalidAbilityId ||
					!ContainsId(abilityIds, abilityId))
				{
					outError =
						"Ability input binding references unknown ability.";
					return false;
				}
			}
		}
	}

	for (const AbilityAnimationBindingProfileDef& profile : animationProfiles)
	{
		if (profile.id == InvalidAbilityAnimationBindingProfileId ||
			!animationProfileIds.insert(profile.id).second)
		{
			outError = "Invalid or duplicate ability animation profile id.";
			return false;
		}

		if (profile.key.empty())
		{
			outError = "Ability animation profile key must not be empty.";
			return false;
		}

		for (const AbilityAnimationBindingDef& binding :
			profile.abilityBindings)
		{
			if (binding.abilityId == InvalidAbilityId ||
				!ContainsId(abilityIds, binding.abilityId))
			{
				outError = "Ability animation binding references unknown ability.";
				return false;
			}

			if (binding.animationKey.empty())
			{
				outError = "Ability animation binding key must not be empty.";
				return false;
			}

			AnimationId animationId{ AnimationId::None };
			if (!ParseAnimationIdString(binding.animationKey, animationId))
			{
				outError =
					"Ability animation binding references unknown animation.";
				return false;
			}
		}

		for (const AbilityLocomotionAnimationBindingDef& binding :
			profile.locomotionBindings)
		{
			if (binding.animationKey.empty())
			{
				outError =
					"Ability locomotion animation binding key must not be empty.";
				return false;
			}

			AnimationId animationId{ AnimationId::None };
			if (!ParseAnimationIdString(binding.animationKey, animationId))
			{
				outError =
					"Ability locomotion animation binding references unknown animation.";
				return false;
			}
		}
	}

	for (const AbilityFallbackReactionProfileDef& profile : fallbackProfiles)
	{
		if (profile.id == InvalidAbilityFallbackReactionProfileId ||
			!fallbackProfileIds.insert(profile.id).second)
		{
			outError = "Invalid or duplicate ability fallback profile id.";
			return false;
		}

		if (profile.key.empty())
		{
			outError = "Ability fallback profile key must not be empty.";
			return false;
		}

		for (const AbilityFallbackReactionEntryDef& entry : profile.entries)
		{
			if (entry.toAbilityId == InvalidAbilityId ||
				!ContainsId(abilityIds, entry.toAbilityId))
			{
				outError =
					"Ability fallback reaction references unknown ability.";
				return false;
			}
		}
	}

	for (const AbilitySetDef& set : sets)
	{
		if (set.id == InvalidAbilitySetId)
		{
			outError = "Ability set id must not be invalid.";
			return false;
		}

		if (set.key.empty() || set.name.empty())
		{
			outError = "Ability set key and name must not be empty.";
			return false;
		}

		if (!setIds.insert(set.id).second)
		{
			outError = "Duplicate ability set id detected.";
			return false;
		}

		for (const AbilityGrantDef& grant : set.grants)
		{
			if (grant.abilityId == InvalidAbilityId ||
				!ContainsId(abilityIds, grant.abilityId))
			{
				outError = "Ability set grant references unknown ability.";
				return false;
			}
		}

		if (set.inputBindingProfileId.has_value() &&
			!ContainsId(inputProfileIds, *set.inputBindingProfileId))
		{
			outError = "Ability set references unknown input binding profile.";
			return false;
		}

		if (set.animationBindingProfileId.has_value() &&
			!ContainsId(animationProfileIds, *set.animationBindingProfileId))
		{
			outError = "Ability set references unknown animation profile.";
			return false;
		}

		if (set.fallbackReactionProfileId.has_value() &&
			!ContainsId(fallbackProfileIds, *set.fallbackReactionProfileId))
		{
			outError = "Ability set references unknown fallback profile.";
			return false;
		}
	}

	return true;
}
