#include "pch.h"
#include "ActionDef.h"

#include "DefEnumString.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"

#include <stdexcept>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace
{
	struct ActionDefTraits
	{
		static ActionId GetId(const ActionDef& def) noexcept
		{
			return def.id;
		}
	};

	struct CharacterActionProfileDefTraits
	{
		static CharacterActionProfileId GetId(
			const CharacterActionProfileDef& def) noexcept
		{
			return def.id;
		}
	};

	struct ActionInputBindingProfileDefTraits
	{
		static ActionInputBindingProfileId GetId(
			const ActionInputBindingProfileDef& def) noexcept
		{
			return def.id;
		}
	};

	struct ActionFallbackReactionProfileDefTraits
	{
		static ActionFallbackReactionProfileId GetId(
			const ActionFallbackReactionProfileDef& def) noexcept
		{
			return def.id;
		}
	};

	struct AnimationBindingProfileDefTraits
	{
		static AnimationBindingProfileId GetId(
			const AnimationBindingProfileDef& def) noexcept
		{
			return def.id;
		}
	};

	using ActionDefRegistry =
		DefRegistry<ActionDef, ActionId, ActionDefTraits>;
	using CharacterActionProfileDefRegistry = DefRegistry<
		CharacterActionProfileDef,
		CharacterActionProfileId,
		CharacterActionProfileDefTraits>;
	using ActionInputBindingProfileDefRegistry = DefRegistry<
		ActionInputBindingProfileDef,
		ActionInputBindingProfileId,
		ActionInputBindingProfileDefTraits>;
	using ActionFallbackReactionProfileDefRegistry = DefRegistry<
		ActionFallbackReactionProfileDef,
		ActionFallbackReactionProfileId,
		ActionFallbackReactionProfileDefTraits>;
	using AnimationBindingProfileDefRegistry = DefRegistry<
		AnimationBindingProfileDef,
		AnimationBindingProfileId,
		AnimationBindingProfileDefTraits>;

	struct ActionDefinitionRegistries
	{
		ActionDefRegistry actions;
		CharacterActionProfileDefRegistry characterActionProfiles;
		ActionInputBindingProfileDefRegistry inputBindingProfiles;
		ActionFallbackReactionProfileDefRegistry fallbackReactionProfiles;
		AnimationBindingProfileDefRegistry animationBindingProfiles;
		std::unordered_map<
			CharacterId,
			CharacterActionProfileId,
			DefRegistryIdHash<CharacterId>> characterProfileByCharacter;
	};

	struct ParsedActionDefinitions
	{
		std::vector<ActionDef> actions;
		std::vector<CharacterActionProfileDef> characterActionProfiles;
		std::vector<ActionInputBindingProfileDef> inputBindingProfiles;
		std::vector<ActionFallbackReactionProfileDef> fallbackReactionProfiles;
		std::vector<AnimationBindingProfileDef> animationBindingProfiles;
	};

	ActionDefinitionRegistries& GetActionDefinitionRegistries() noexcept
	{
		static ActionDefinitionRegistries registries;
		return registries;
	}

	template<typename TEnum>
	bool ReadEnum(
		const json& node,
		const char* field,
		TEnum& outValue,
		const char* typeName,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!ParseDefString(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	template<typename TEnum>
	bool ReadNullableEnum(
		const json& node,
		const char* field,
		std::optional<TEnum>& outValue,
		const char* typeName,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		TEnum value{};
		if (!ReadEnum(node, field, value, typeName, outError))
			return false;

		outValue = value;
		return true;
	}

	template<typename TValue>
	bool ReadNullableNumber(
		const json& node,
		const char* field,
		std::optional<TValue>& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at(field).is_number())
		{
			outError = std::string("Invalid numeric field: ") + field;
			return false;
		}

		outValue = node.at(field).get<TValue>();
		return true;
	}

	template<typename TId, typename TParser>
	bool ReadNamedId(
		const json& node,
		const char* field,
		TId& outValue,
		const char* typeName,
		TParser parser,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!parser(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	template<typename TDef, typename TParser>
	bool ParseRequiredArray(
		const json& node,
		const char* field,
		std::vector<TDef>& outValues,
		TParser parser,
		std::string& outError)
	{
		if (!node.contains(field) || !node.at(field).is_array())
		{
			outError = std::string("Missing or invalid array field: ") + field;
			return false;
		}

		const json& arrayNode = node.at(field);
		outValues.clear();
		outValues.reserve(arrayNode.size());
		for (const json& item : arrayNode)
		{
			TDef value{};
			if (!parser(item, value, outError))
				return false;

			outValues.push_back(std::move(value));
		}

		return true;
	}

	bool ParseActionIdArray(
		const json& node,
		const char* field,
		std::vector<ActionId>& outValues,
		std::string& outError)
	{
		if (!node.contains(field) || !node.at(field).is_array())
		{
			outError = std::string("Missing or invalid array field: ") + field;
			return false;
		}

		const json& arrayNode = node.at(field);
		outValues.clear();
		outValues.reserve(arrayNode.size());
		for (const json& item : arrayNode)
		{
			if (!item.is_string())
			{
				outError = std::string(field) + " entry must be a string.";
				return false;
			}

			ActionId id{};
			const std::string text = item.get<std::string>();
			if (!ParseDefString(text, id))
			{
				outError = "Unknown ActionId: " + text;
				return false;
			}

			outValues.push_back(id);
		}

		return true;
	}

	bool ParseEndPolicy(
		const json& node,
		ActionEndPolicyDef& outPolicy,
		std::string& outError)
	{
		return
			ReadEnum(node, "endType", outPolicy.endType, "ActionEndType", outError) &&
			ReadEnum(node, "defaultNextActionId", outPolicy.defaultNextActionId, "ActionId", outError);
	}

	bool ParseRequestRequirement(
		const json& node,
		ActionRequestRequirementDef& outRequirement,
		std::string& outError)
	{
		return
			ReadEnum(node, "type", outRequirement.type, "ActionRequestRequirementType", outError) &&
			ReadNullableNumber(node, "scalar", outRequirement.scalar, outError) &&
			ReadNullableEnum(node, "stateFlag", outRequirement.stateFlag, "GameplayStateFlag", outError);
	}

	bool ParseResourceCost(
		const json& node,
		ActionResourceCostDef& outCost,
		std::string& outError)
	{
		return
			ReadEnum(node, "type", outCost.type, "ActionResourceType", outError) &&
			ReadEnum(node, "consumeTiming", outCost.consumeTiming, "ActionResourceConsumeTiming", outError) &&
			ReadRequiredNumber(node, "amount", outCost.amount, outError);
	}

	bool ParseInterruptRule(
		const json& node,
		ActionInterruptRule& outRule,
		std::string& outError)
	{
		return
			ReadEnum(node, "causeType", outRule.causeType, "ActionInterruptCauseType", outError) &&
			ReadEnum(node, "toActionId", outRule.toActionId, "ActionId", outError) &&
			ReadEnum(node, "windowPolicy", outRule.windowPolicy, "ActionWindowPolicy", outError) &&
			ReadNullableNumber(node, "windowStartNormalized", outRule.windowStartNormalized, outError) &&
			ReadNullableNumber(node, "windowEndNormalized", outRule.windowEndNormalized, outError) &&
			ReadRequiredNumber(node, "priority", outRule.priority, outError);
	}

	bool ParseCancelRule(
		const json& node,
		ActionCancelRule& outRule,
		std::string& outError)
	{
		if (!ReadEnum(node, "cancelKind", outRule.cancelKind, "ActionCancelKind", outError) ||
			!ReadEnum(node, "toActionId", outRule.toActionId, "ActionId", outError) ||
			!ReadEnum(node, "windowPolicy", outRule.windowPolicy, "ActionWindowPolicy", outError) ||
			!ReadNullableNumber(node, "windowStartNormalized", outRule.windowStartNormalized, outError) ||
			!ReadNullableNumber(node, "windowEndNormalized", outRule.windowEndNormalized, outError) ||
			!ReadRequiredNumber(node, "priority", outRule.priority, outError))
		{
			return false;
		}

		if (node.contains("aiInterruptible") &&
			!ReadRequiredBool(node, "aiInterruptible", outRule.aiInterruptible, outError))
		{
			return false;
		}

		return true;
	}

	bool ParseTransitionRule(
		const json& node,
		ActionTransitionRuleDef& outRule,
		std::string& outError)
	{
		return
			ParseRequiredArray<ActionInterruptRule>(
				node,
				"interruptRules",
				outRule.interruptRules,
				ParseInterruptRule,
				outError) &&
			ParseRequiredArray<ActionCancelRule>(
				node,
				"cancelRules",
				outRule.cancelRules,
				ParseCancelRule,
				outError);
	}

	bool ParseSpatialFilter(
		const json& node,
		std::optional<ActionCombatSpatialFilterDef>& outFilter,
		std::string& outError)
	{
		if (node.is_null())
		{
			outFilter = std::nullopt;
			return true;
		}

		ActionCombatSpatialFilterDef filter{};
		if (!ReadNullableNumber(node, "facingHalfAngleDeg", filter.facingHalfAngleDeg, outError) ||
			!ReadNullableNumber(node, "minDistance", filter.minDistance, outError) ||
			!ReadNullableNumber(node, "maxDistance", filter.maxDistance, outError) ||
			!ReadNullableNumber(node, "verticalTolerance", filter.verticalTolerance, outError))
		{
			return false;
		}

		if (node.contains("referenceFrame") &&
			!ReadEnum(node, "referenceFrame", filter.referenceFrame, "CombatReferenceFrame", outError))
		{
			return false;
		}

		outFilter = filter;
		return true;
	}

	bool ParseCombatEffect(
		const json& node,
		std::optional<CombatEffectDef>& outEffect,
		std::string& outError)
	{
		if (node.is_null())
		{
			outEffect = std::nullopt;
			return true;
		}

		CombatEffectDef effect{};
		if (!ReadEnum(node, "type", effect.type, "CombatEffectType", outError))
			return false;

		if (node.contains("attackHit") && !node.at("attackHit").is_null())
		{
			const json& attackNode = node.at("attackHit");
			AttackCombatEffectDef attack{};
			if (!ReadRequiredNumber(attackNode, "damageScale", attack.damageScale, outError) ||
				!ReadRequiredNumber(attackNode, "bonusDamage", attack.bonusDamage, outError) ||
				!ReadRequiredNumber(attackNode, "staminaDamageScale", attack.staminaDamageScale, outError) ||
				!ReadRequiredNumber(attackNode, "bonusStaminaDamage", attack.bonusStaminaDamage, outError) ||
				!ReadRequiredNumber(attackNode, "poiseDamageScale", attack.poiseDamageScale, outError) ||
				!ReadRequiredNumber(attackNode, "bonusPoiseDamage", attack.bonusPoiseDamage, outError) ||
				!ReadRequiredNumber(attackNode, "knockbackDistance", attack.knockbackDistance, outError) ||
				!ReadRequiredNumber(attackNode, "hitStopSec", attack.hitStopSec, outError) ||
				!ReadRequiredBool(attackNode, "parryable", attack.parryable, outError) ||
				!ReadRequiredBool(attackNode, "guardable", attack.guardable, outError))
			{
				return false;
			}
			effect.attackHit = attack;
		}

		if (node.contains("parryResponse") && !node.at("parryResponse").is_null())
		{
			const json& parryNode = node.at("parryResponse");
			ParryCombatEffectDef parry{};
			if (!ReadRequiredNumber(parryNode, "stunSec", parry.stunSec, outError) ||
				!ReadRequiredNumber(parryNode, "hitStopSec", parry.hitStopSec, outError) ||
				!ReadNullableEnum(parryNode, "grantBuffId", parry.grantBuffId, "BuffId", outError))
			{
				return false;
			}
			effect.parryResponse = parry;
		}

		if (node.contains("guardResponse") && !node.at("guardResponse").is_null())
		{
			const json& guardNode = node.at("guardResponse");
			GuardCombatEffectDef guard{};
			if (!ReadRequiredNumber(guardNode, "damageReductionRatio", guard.damageReductionRatio, outError) ||
				!ReadRequiredNumber(guardNode, "chipDamageRatio", guard.chipDamageRatio, outError) ||
				!ReadRequiredNumber(guardNode, "staminaDamageMultiplier", guard.staminaDamageMultiplier, outError) ||
				!ReadRequiredNumber(guardNode, "hitStopSec", guard.hitStopSec, outError))
			{
				return false;
			}
			effect.guardResponse = guard;
		}

		outEffect = effect;
		return true;
	}

	bool ParseCombatWindow(
		const json& node,
		ActionCombatWindowDef& outWindow,
		std::string& outError)
	{
		if (!ReadEnum(node, "windowType", outWindow.windowType, "CombatWindowType", outError) ||
			!ReadRequiredNumber(node, "startNormalized", outWindow.startNormalized, outError) ||
			!ReadRequiredNumber(node, "endNormalized", outWindow.endNormalized, outError) ||
			!ReadNullableEnum(node, "appliesTo", outWindow.appliesTo, "ActionCombatApplyTo", outError))
		{
			return false;
		}

		if (!node.contains("spatialFilter") ||
			!ParseSpatialFilter(node.at("spatialFilter"), outWindow.spatialFilter, outError))
		{
			return false;
		}

		return node.contains("effect") &&
			ParseCombatEffect(node.at("effect"), outWindow.effect, outError);
	}

	bool ParseEvent(
		const json& node,
		ActionEventDef& outEvent,
		std::string& outError)
	{
		return
			ReadEnum(node, "type", outEvent.type, "EventType", outError) &&
			ReadRequiredNumber(node, "timeNormalized", outEvent.timeNormalized, outError) &&
			ReadNullableNumber(node, "payloadId", outEvent.payloadId, outError) &&
			ReadEnum(node, "conditionType", outEvent.conditionType, "TriggerConditionType", outError);
	}

	bool ParseMoveSegment(
		const json& node,
		ActionMovementSegmentDef& outSegment,
		std::string& outError)
	{
		return
			ReadRequiredNumber(node, "startNormalized", outSegment.startNormalized, outError) &&
			ReadRequiredNumber(node, "endNormalized", outSegment.endNormalized, outError) &&
			ReadEnum(node, "horizontalMoveMode", outSegment.horizontalMoveMode, "HorizontalMovementMode", outError) &&
			ReadNullableNumber(node, "moveDistance", outSegment.moveDistance, outError) &&
			ReadEnum(node, "rotationMode", outSegment.rotationMode, "RotationMode", outError) &&
			ReadNullableNumber(node, "rotationRate", outSegment.rotationRate, outError) &&
			ReadEnum(node, "verticalMoveMode", outSegment.verticalMoveMode, "VerticalMovementMode", outError) &&
			ReadNullableNumber(node, "verticalAmount", outSegment.verticalAmount, outError) &&
			ReadEnum(node, "dirPolicy", outSegment.dirPolicy, "DirectionPolicy", outError) &&
			ReadEnum(node, "dirSampleTiming", outSegment.dirSampleTiming, "DirectionSampleTiming", outError);
	}

	bool ParseAction(
		const json& node,
		ActionDef& outDef,
		std::string& outError)
	{
		if (!ReadEnum(node, "id", outDef.id, "ActionId", outError) ||
			!ReadRequiredString(node, "name", outDef.name, outError) ||
			!ReadEnum(node, "kind", outDef.kind, "ActionKind", outError) ||
			!ReadRequiredNumber(node, "duration", outDef.duration, outError) ||
			!ReadEnum(node, "normalizedPolicy", outDef.normalizedPolicy, "ActionNormalizedPolicy", outError))
		{
			return false;
		}

		if (node.contains("tags") &&
			!ReadRequiredNumber(node, "tags", outDef.tags, outError))
		{
			return false;
		}

		if (!node.contains("endPolicy") ||
			!ParseEndPolicy(node.at("endPolicy"), outDef.endPolicy, outError))
		{
			return false;
		}

		if (!ParseRequiredArray<ActionRequestRequirementDef>(
				node,
				"requestRequirements",
				outDef.requestRequirements,
				ParseRequestRequirement,
				outError) ||
			!ParseRequiredArray<ActionResourceCostDef>(
				node,
				"resourceCosts",
				outDef.resourceCosts,
				ParseResourceCost,
				outError))
		{
			return false;
		}

		if (!node.contains("transitionRule") ||
			!ParseTransitionRule(node.at("transitionRule"), outDef.transitionRule, outError))
		{
			return false;
		}

		return
			ParseRequiredArray<ActionCombatWindowDef>(
				node,
				"combatWindows",
				outDef.combatWindows,
				ParseCombatWindow,
				outError) &&
			ParseRequiredArray<ActionEventDef>(
				node,
				"events",
				outDef.events,
				ParseEvent,
				outError) &&
			ParseRequiredArray<ActionMovementSegmentDef>(
				node,
				"moveSegments",
				outDef.moveSegments,
				ParseMoveSegment,
				outError);
	}

	bool ParseInputBindingEntry(
		const json& node,
		ActionInputBindingEntryDef& outEntry,
		std::string& outError)
	{
		return
			ReadEnum(node, "request", outEntry.request, "ActionRequestSemantic", outError) &&
			ParseActionIdArray(node, "candidateActions", outEntry.candidateActions, outError) &&
			ReadEnum(node, "selectionPolicy", outEntry.selectionPolicy, "ActionCandidateSelectionPolicy", outError) &&
			ReadRequiredNumber(node, "priority", outEntry.priority, outError);
	}

	bool ParseInputBindingProfile(
		const json& node,
		ActionInputBindingProfileDef& outProfile,
		std::string& outError)
	{
		return
			ReadNamedId(
				node,
				"id",
				outProfile.id,
				"ActionInputBindingProfileId",
				ParseActionInputBindingProfileIdString,
				outError) &&
			ParseRequiredArray<ActionInputBindingEntryDef>(
				node,
				"entries",
				outProfile.entries,
				ParseInputBindingEntry,
				outError);
	}

	bool ParseFallbackReactionEntry(
		const json& node,
		ActionFallbackReactionEntryDef& outEntry,
		std::string& outError)
	{
		return
			ReadEnum(node, "causeType", outEntry.causeType, "ActionInterruptCauseType", outError) &&
			ReadEnum(node, "toActionId", outEntry.toActionId, "ActionId", outError) &&
			ReadRequiredNumber(node, "priority", outEntry.priority, outError);
	}

	bool ParseFallbackReactionProfile(
		const json& node,
		ActionFallbackReactionProfileDef& outProfile,
		std::string& outError)
	{
		return
			ReadNamedId(
				node,
				"id",
				outProfile.id,
				"ActionFallbackReactionProfileId",
				ParseActionFallbackReactionProfileIdString,
				outError) &&
			ParseRequiredArray<ActionFallbackReactionEntryDef>(
				node,
				"entries",
				outProfile.entries,
				ParseFallbackReactionEntry,
				outError);
	}

	bool ParseCharacterActionProfile(
		const json& node,
		CharacterActionProfileDef& outProfile,
		std::string& outError)
	{
		return
			ReadNamedId(
				node,
				"id",
				outProfile.id,
				"CharacterActionProfileId",
				ParseCharacterActionProfileIdString,
				outError) &&
			ReadEnum(node, "characterId", outProfile.characterId, "CharacterId", outError) &&
			ParseActionIdArray(node, "availableActions", outProfile.availableActions, outError) &&
			ReadNamedId(
				node,
				"inputBindingProfileId",
				outProfile.inputBindingProfileId,
				"ActionInputBindingProfileId",
				ParseActionInputBindingProfileIdString,
				outError) &&
			ReadNamedId(
				node,
				"fallbackReactionProfileId",
				outProfile.fallbackReactionProfileId,
				"ActionFallbackReactionProfileId",
				ParseActionFallbackReactionProfileIdString,
				outError) &&
			ReadNamedId(
				node,
				"animationBindingProfileId",
				outProfile.animationBindingProfileId,
				"AnimationBindingProfileId",
				ParseAnimationBindingProfileIdString,
				outError);
	}

	bool ParseActionAnimationBinding(
		const json& node,
		ActionAnimationBindingDef& outBinding,
		std::string& outError)
	{
		if (!ReadEnum(node, "actionId", outBinding.actionId, "ActionId", outError) ||
			!ReadNamedId(node, "animationId", outBinding.animationId, "AnimationId", ParseAnimationIdString, outError))
		{
			return false;
		}

		if (node.contains("playRate") &&
			!ReadRequiredNumber(node, "playRate", outBinding.playRate, outError))
		{
			return false;
		}

		if (node.contains("startNormalizedTime") &&
			!ReadRequiredNumber(node, "startNormalizedTime", outBinding.startNormalizedTime, outError))
		{
			return false;
		}

		return true;
	}

	bool ParseLocomotionAnimationBinding(
		const json& node,
		LocomotionAnimationBindingDef& outBinding,
		std::string& outError)
	{
		if (!ReadEnum(node, "mode", outBinding.mode, "LocomotionMode", outError) ||
			!ReadNamedId(node, "animationId", outBinding.animationId, "AnimationId", ParseAnimationIdString, outError))
		{
			return false;
		}

		if (node.contains("playRate") &&
			!ReadRequiredNumber(node, "playRate", outBinding.playRate, outError))
		{
			return false;
		}

		if (node.contains("holdLastFrame") &&
			!ReadRequiredBool(node, "holdLastFrame", outBinding.holdLastFrame, outError))
		{
			return false;
		}

		return true;
	}

	bool ParseAnimationBindingProfile(
		const json& node,
		AnimationBindingProfileDef& outProfile,
		std::string& outError)
	{
		return
			ReadNamedId(
				node,
				"id",
				outProfile.id,
				"AnimationBindingProfileId",
				ParseAnimationBindingProfileIdString,
				outError) &&
			ParseRequiredArray<ActionAnimationBindingDef>(
				node,
				"actionBindings",
				outProfile.actionBindings,
				ParseActionAnimationBinding,
				outError) &&
			ParseRequiredArray<LocomotionAnimationBindingDef>(
				node,
				"locomotionBindings",
				outProfile.locomotionBindings,
				ParseLocomotionAnimationBinding,
				outError);
	}

	bool ParseActionDocument(
		const json& root,
		ParsedActionDefinitions& outDefs,
		std::string& outError)
	{
		return
			ParseRequiredArray<ActionDef>(
				root,
				"actions",
				outDefs.actions,
				ParseAction,
				outError) &&
			ParseRequiredArray<CharacterActionProfileDef>(
				root,
				"characterActionProfiles",
				outDefs.characterActionProfiles,
				ParseCharacterActionProfile,
				outError) &&
			ParseRequiredArray<ActionInputBindingProfileDef>(
				root,
				"inputBindingProfiles",
				outDefs.inputBindingProfiles,
				ParseInputBindingProfile,
				outError) &&
			ParseRequiredArray<ActionFallbackReactionProfileDef>(
				root,
				"fallbackReactionProfiles",
				outDefs.fallbackReactionProfiles,
				ParseFallbackReactionProfile,
				outError) &&
			ParseRequiredArray<AnimationBindingProfileDef>(
				root,
				"animationBindingProfiles",
				outDefs.animationBindingProfiles,
				ParseAnimationBindingProfile,
				outError);
	}

	bool BuildRegistries(
		ParsedActionDefinitions defs,
		ActionDefinitionRegistries& outRegistries,
		std::string& outError)
	{
		if (!outRegistries.actions.Build(std::move(defs.actions), &outError) ||
			!outRegistries.characterActionProfiles.Build(
				std::move(defs.characterActionProfiles),
				&outError) ||
			!outRegistries.inputBindingProfiles.Build(
				std::move(defs.inputBindingProfiles),
				&outError) ||
			!outRegistries.fallbackReactionProfiles.Build(
				std::move(defs.fallbackReactionProfiles),
				&outError) ||
			!outRegistries.animationBindingProfiles.Build(
				std::move(defs.animationBindingProfiles),
				&outError))
		{
			return false;
		}

		outRegistries.characterProfileByCharacter.clear();
		outRegistries.characterProfileByCharacter.reserve(
			outRegistries.characterActionProfiles.Size());
		for (const CharacterActionProfileDef& def :
			outRegistries.characterActionProfiles.GetAll())
		{
			const auto [it, inserted] =
				outRegistries.characterProfileByCharacter.try_emplace(
					def.characterId,
					def.id);
			(void)it;
			if (!inserted)
			{
				outError = "Duplicate CharacterActionProfileDef characterId detected.";
				return false;
			}
		}

		return true;
	}
}

const ActionDef* FindActionDef(ActionId id) noexcept
{
	return GetActionDefinitionRegistries().actions.Find(id);
}

const ActionDef& GetActionDef(ActionId id)
{
	return GetActionDefinitionRegistries().actions.Get(id);
}

std::span<const ActionDef> GetActionDefs() noexcept
{
	return GetActionDefinitionRegistries().actions.GetAll();
}

const CharacterActionProfileDef* FindCharacterActionProfileDef(
	CharacterActionProfileId id) noexcept
{
	return GetActionDefinitionRegistries().characterActionProfiles.Find(id);
}

const CharacterActionProfileDef& GetCharacterActionProfileDef(
	CharacterActionProfileId id)
{
	return GetActionDefinitionRegistries().characterActionProfiles.Get(id);
}

const CharacterActionProfileDef* FindCharacterActionProfileDefByCharacter(
	CharacterId characterId) noexcept
{
	const ActionDefinitionRegistries& registries = GetActionDefinitionRegistries();
	const auto it = registries.characterProfileByCharacter.find(characterId);
	if (it == registries.characterProfileByCharacter.end())
	{
		return nullptr;
	}

	return registries.characterActionProfiles.Find(it->second);
}

std::span<const CharacterActionProfileDef> GetCharacterActionProfileDefs() noexcept
{
	return GetActionDefinitionRegistries().characterActionProfiles.GetAll();
}

const ActionInputBindingProfileDef* FindActionInputBindingProfileDef(
	ActionInputBindingProfileId id) noexcept
{
	return GetActionDefinitionRegistries().inputBindingProfiles.Find(id);
}

const ActionInputBindingProfileDef& GetActionInputBindingProfileDef(
	ActionInputBindingProfileId id)
{
	return GetActionDefinitionRegistries().inputBindingProfiles.Get(id);
}

std::span<const ActionInputBindingProfileDef> GetActionInputBindingProfileDefs() noexcept
{
	return GetActionDefinitionRegistries().inputBindingProfiles.GetAll();
}

const ActionFallbackReactionProfileDef* FindActionFallbackReactionProfileDef(
	ActionFallbackReactionProfileId id) noexcept
{
	return GetActionDefinitionRegistries().fallbackReactionProfiles.Find(id);
}

const ActionFallbackReactionProfileDef& GetActionFallbackReactionProfileDef(
	ActionFallbackReactionProfileId id)
{
	return GetActionDefinitionRegistries().fallbackReactionProfiles.Get(id);
}

std::span<const ActionFallbackReactionProfileDef> GetActionFallbackReactionProfileDefs() noexcept
{
	return GetActionDefinitionRegistries().fallbackReactionProfiles.GetAll();
}

const AnimationBindingProfileDef* FindAnimationBindingProfileDef(
	AnimationBindingProfileId id) noexcept
{
	return GetActionDefinitionRegistries().animationBindingProfiles.Find(id);
}

const AnimationBindingProfileDef& GetAnimationBindingProfileDef(
	AnimationBindingProfileId id)
{
	return GetActionDefinitionRegistries().animationBindingProfiles.Get(id);
}

std::span<const AnimationBindingProfileDef> GetAnimationBindingProfileDefs() noexcept
{
	return GetActionDefinitionRegistries().animationBindingProfiles.GetAll();
}

ActionDefLoadResult LoadActionDefsFromJsonDirectory(
	const std::filesystem::path& directory)
{
	std::vector<DefJsonDocument> documents;
	ActionDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "Action");
	if (!result.succeeded)
		return result;

	ParsedActionDefinitions defs;
	for (const DefJsonDocument& document : documents)
	{
		try
		{
			if (!ParseActionDocument(document.root, defs, result.error))
			{
				result.error = document.path.string() + ": " + result.error;
				result.succeeded = false;
				return result;
			}
		}
		catch (const std::exception& ex)
		{
			result.error = document.path.string() +
				": Invalid action json field: " + std::string(ex.what());
			result.succeeded = false;
			return result;
		}
	}

	ActionDefinitionRegistries registries;
	if (!BuildRegistries(std::move(defs), registries, result.error))
	{
		result.succeeded = false;
		return result;
	}

	ActionDefinitionRegistries& activeRegistries = GetActionDefinitionRegistries();
	activeRegistries = std::move(registries);
	result.succeeded = true;
	result.loadedCount = activeRegistries.actions.Size();
	result.error.clear();
	return result;
}
