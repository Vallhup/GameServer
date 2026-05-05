#include "pch.h"
#include "AIBehaviorDef.h"

#include "DefCompilePipeline.h"
#include "DefEnumString.h"
#include "DefJsonReader.h"
#include "GameplayContentCatalog.h"
#include "DefRegistry.h"
#include "json.hpp"

#include <vector>

using json = nlohmann::json;

namespace
{
	struct AIBehaviorProfileDto
	{
		AIBehaviorProfileDef profile;
		std::vector<WeightedActionEntry> combatActions;
		std::vector<WeightedActionEntry> idleActions;
		std::vector<AIBossPhaseTransitionDef> bossPhaseTransitions;
	};

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

	bool ReadAIProfileKey(
		const json& node,
		const char* field,
		std::string& outKey,
		AIBehaviorProfileId& outId,
		std::string& outError)
	{
		if (!ReadRequiredString(node, field, outKey, outError))
			return false;

		outId = HashDefKey(outKey);
		if (outId == InvalidAIBehaviorProfileId)
		{
			outError = "Invalid AI profile key: " + outKey;
			return false;
		}

		return true;
	}

	bool ParsePerceptionTuning(
		const json& node,
		AIPerceptionTuningDef& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "sightRange", outTuning.sightRange, outError) &&
			ReadOptionalNumber(node, "attackRange", outTuning.attackRange, outError) &&
			ReadOptionalNumber(node, "frontDotThreshold", outTuning.frontDotThreshold, outError) &&
			ReadOptionalNumber(node, "targetKeepBonus", outTuning.targetKeepBonus, outError) &&
			ReadOptionalNumber(node, "lastAttackerBonus", outTuning.lastAttackerBonus, outError) &&
			ReadOptionalNumber(node, "frontBonus", outTuning.frontBonus, outError) &&
			ReadOptionalNumber(node, "switchScoreMargin", outTuning.switchScoreMargin, outError) &&
			ReadOptionalNumber(node, "loseSightGraceTime", outTuning.loseSightGraceTime, outError) &&
			ReadOptionalNumber(node, "leashRange", outTuning.leashRange, outError) &&
			ReadOptionalNumber(node, "hardLeashRange", outTuning.hardLeashRange, outError) &&
			ReadOptionalNumber(node, "leashGaugeMax", outTuning.leashGaugeMax, outError) &&
			ReadOptionalNumber(node, "leashDrainPerSec", outTuning.leashDrainPerSec, outError) &&
			ReadOptionalNumber(node, "hardLeashDrainPerSec", outTuning.hardLeashDrainPerSec, outError) &&
			ReadOptionalNumber(node, "leashRecoverPerSec", outTuning.leashRecoverPerSec, outError) &&
			ReadOptionalNumber(node, "returnHomeArriveRange", outTuning.returnHomeArriveRange, outError) &&
			ReadOptionalNumber(node, "returnHomeReaggroLockSec", outTuning.returnHomeReaggroLockSec, outError) &&
			ReadOptionalNumber(node, "returnHpRegenPerSecRatio", outTuning.returnHpRegenPerSecRatio, outError) &&
			ReadOptionalNumber(node, "idleActionCooldownSec", outTuning.idleActionCooldownSec, outError) &&
			ReadOptionalNumber(node, "idleActionChancePercent", outTuning.idleActionChancePercent, outError) &&
			ReadOptionalNumber(node, "assistRange", outTuning.assistRange, outError);
	}

	bool ParseDecisionTuning(
		const json& node,
		AIDecisionTuningDef& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "decisionInterval", outTuning.decisionInterval, outError) &&
			ReadOptionalNumber(node, "attackCooldown", outTuning.attackCooldown, outError) &&
			ReadOptionalNumber(node, "reactDuration", outTuning.reactDuration, outError);
	}

	bool ReadOptionalBool(
		const json& node,
		const char* field,
		bool& outValue,
		std::string& outError)
	{
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_boolean())
		{
			outError = std::string("Invalid bool field: ") + field;
			return false;
		}

		outValue = node.at(field).get<bool>();
		return true;
	}

	bool ParseDistanceBucket(
		std::string_view text,
		AIActionDistanceBucket& outValue) noexcept
	{
		if (text == "Any") { outValue = AIActionDistanceBucket::Any; return true; }
		if (text == "VeryClose") { outValue = AIActionDistanceBucket::VeryClose; return true; }
		if (text == "Close") { outValue = AIActionDistanceBucket::Close; return true; }
		if (text == "Mid") { outValue = AIActionDistanceBucket::Mid; return true; }
		if (text == "Far") { outValue = AIActionDistanceBucket::Far; return true; }
		return false;
	}

	bool ParseWeightedActions(
		const json& node,
		std::vector<WeightedActionEntry>& outAbilities,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Weighted ability list must be an array.";
			return false;
		}

		outAbilities.clear();
		outAbilities.reserve(node.size());
		for (const json& abilityNode : node)
		{
			WeightedActionEntry entry{};
			std::string abilityKey;
			if (!ReadRequiredString(
				abilityNode,
				"abilityKey",
				abilityKey,
				outError))
			{
				return false;
			}

			const GameplayContentCatalogSnapshot* catalog =
				GameplayContentCatalogSnapshot::TryCurrent();
			const AbilityDef* ability =
				catalog != nullptr ? catalog->FindAbilityByKey(abilityKey) : nullptr;
			if (ability == nullptr)
			{
				outError = "Unknown ability key: " + abilityKey;
				return false;
			}
			entry.abilityId = ability->id;

			if (!ReadOptionalNumber(abilityNode, "weight", entry.weight, outError))
				return false;

			if (!ReadOptionalNumber(abilityNode, "aiCooldownSec", entry.aiCooldownSec, outError))
				return false;

			if (!ReadOptionalBool(
				abilityNode,
				"forbidImmediateRepeat",
				entry.forbidImmediateRepeat,
				outError))
			{
				return false;
			}

			if (abilityNode.contains("distanceBucket"))
			{
				std::string distanceBucket;
				if (!ReadRequiredString(
					abilityNode,
					"distanceBucket",
					distanceBucket,
					outError))
				{
					return false;
				}

				if (!ParseDistanceBucket(distanceBucket, entry.distanceBucket))
				{
					outError = "Unknown AI action distance bucket: " + distanceBucket;
					return false;
				}
			}

			outAbilities.push_back(entry);
		}

		return true;
	}

	bool ParseBossMovementTuning(
		const json& node,
		AIBossMovementTuningDef& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "veryCloseDistance", outTuning.veryCloseDistance, outError) &&
			ReadOptionalNumber(node, "closeDistance", outTuning.closeDistance, outError) &&
			ReadOptionalNumber(node, "midDistance", outTuning.midDistance, outError) &&
			ReadOptionalNumber(node, "preferredMinDistance", outTuning.preferredMinDistance, outError) &&
			ReadOptionalNumber(node, "preferredMaxDistance", outTuning.preferredMaxDistance, outError) &&
			ReadOptionalNumber(node, "strafeMinSec", outTuning.strafeMinSec, outError) &&
			ReadOptionalNumber(node, "strafeMaxSec", outTuning.strafeMaxSec, outError);
	}

	bool ParseBossPhaseTransitions(
		const json& node,
		std::vector<AIBossPhaseTransitionDef>& outTransitions,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Boss phaseTransitions must be an array.";
			return false;
		}

		outTransitions.clear();
		outTransitions.reserve(node.size());
		for (const json& transitionNode : node)
		{
			AIBossPhaseTransitionDef transition{};
			if (!ReadRequiredNumber(transitionNode, "phase", transition.phase, outError) ||
				!ReadRequiredNumber(transitionNode, "hpRatio", transition.hpRatio, outError) ||
				!ReadOptionalNumber(
					transitionNode,
					"transitionLockSec",
					transition.transitionLockSec,
					outError))
			{
				return false;
			}

			if (transitionNode.contains("transitionAbilityKey") &&
				!transitionNode.at("transitionAbilityKey").is_null())
			{
				std::string abilityKey;
				if (!ReadRequiredString(
					transitionNode,
					"transitionAbilityKey",
					abilityKey,
					outError))
				{
					return false;
				}

				const GameplayContentCatalogSnapshot* catalog =
					GameplayContentCatalogSnapshot::TryCurrent();
				const AbilityDef* ability =
					catalog != nullptr ? catalog->FindAbilityByKey(abilityKey) : nullptr;
				if (ability == nullptr)
				{
					outError = "Unknown boss transition ability key: " + abilityKey;
					return false;
				}

				transition.transitionAbilityId = ability->id;
			}

			outTransitions.push_back(transition);
		}

		return true;
	}

	bool ParseBossPattern(
		const json& node,
		AIBossPatternProfileDef& outPattern,
		std::vector<AIBossPhaseTransitionDef>& outTransitions,
		std::string& outError)
	{
		if (node.contains("movement") &&
			!ParseBossMovementTuning(
				node.at("movement"),
				outPattern.movement,
				outError))
		{
			return false;
		}

		if (node.contains("phaseTransitions") &&
			!ParseBossPhaseTransitions(
				node.at("phaseTransitions"),
				outTransitions,
				outError))
		{
			return false;
		}

		return true;
	}

	bool ParseProfileDocument(
		const json& root,
		AIBehaviorProfileDef& outProfile,
		std::vector<WeightedActionEntry>& outCombatAbilities,
		std::vector<WeightedActionEntry>& outIdleAbilities,
		std::vector<AIBossPhaseTransitionDef>& outBossPhaseTransitions,
		std::string& outError)
	{
		if (!ReadAIProfileKey(root, "key", outProfile.key, outProfile.id, outError) ||
			!ReadEnum(root, "aiType", outProfile.aiType, "AI archetype", outError))
		{
			return false;
		}

		if (root.contains("perceptionTuning") &&
			!ParsePerceptionTuning(
				root.at("perceptionTuning"),
				outProfile.perception,
				outError))
		{
			return false;
		}

		if (root.contains("decisionTuning") &&
			!ParseDecisionTuning(
				root.at("decisionTuning"),
				outProfile.decision,
				outError))
		{
			return false;
		}

		if (root.contains("combatActions") &&
			!ParseWeightedActions(root.at("combatActions"), outCombatAbilities, outError))
		{
			return false;
		}

		if (root.contains("idleActions") &&
			!ParseWeightedActions(root.at("idleActions"), outIdleAbilities, outError))
		{
			return false;
		}

		if (root.contains("bossPattern") &&
			!ParseBossPattern(
				root.at("bossPattern"),
				outProfile.bossPattern,
				outBossPhaseTransitions,
				outError))
		{
			return false;
		}

		return true;
	}

	bool ValidateLoadedProfiles(
		std::span<const AIBehaviorProfileDef> profiles,
		std::string& outError)
	{
		for (size_t i = 0; i < profiles.size(); ++i)
		{
			const AIBehaviorProfileDef& profile = profiles[i];
			if (profile.id == InvalidAIBehaviorProfileId)
			{
				outError = "AI behavior profile key must not be empty.";
				return false;
			}

			for (size_t j = i + 1; j < profiles.size(); ++j)
			{
				const AIBehaviorProfileDef& other = profiles[j];
				if (profile.id == other.id)
				{
					if (profile.key != other.key)
					{
						outError =
							"AI behavior profile key hash collision: " +
							profile.key + " / " + other.key;
					}
					else
					{
						outError = "Duplicate AI behavior profile key: " + profile.key;
					}
					return false;
				}
			}

			if (profile.combatActions.empty())
			{
				outError = "AI behavior profile requires combatActions.";
				return false;
			}
		}

		return true;
	}

	bool AppendAIBehaviorDocumentDtos(
		const json& root,
		std::vector<AIBehaviorProfileDto>& outDtos,
		std::string& outError)
	{
		AIBehaviorProfileDto dto{};
		if (!ParseProfileDocument(
				root,
				dto.profile,
				dto.combatActions,
				dto.idleActions,
				dto.bossPhaseTransitions,
				outError))
		{
			return false;
		}

		outDtos.push_back(std::move(dto));
		return true;
	}

	bool BuildAIBehaviorDefinitionSet(
		std::span<const DefParsedDto<AIBehaviorProfileDto>> dtos,
		AIBehaviorDefinitionSet& outDefs,
		std::string& outError)
	{
		std::vector<AIBehaviorProfileDef> profiles;
		profiles.reserve(dtos.size());

		outDefs.combatActionStorage.clear();
		outDefs.idleActionStorage.clear();
		outDefs.bossPhaseTransitionStorage.clear();
		outDefs.combatActionStorage.reserve(dtos.size());
		outDefs.idleActionStorage.reserve(dtos.size());
		outDefs.bossPhaseTransitionStorage.reserve(dtos.size());

		for (const DefParsedDto<AIBehaviorProfileDto>& parsedDto : dtos)
		{
			profiles.push_back(parsedDto.dto.profile);
			outDefs.combatActionStorage.push_back(parsedDto.dto.combatActions);
			outDefs.idleActionStorage.push_back(parsedDto.dto.idleActions);
			outDefs.bossPhaseTransitionStorage.push_back(
				parsedDto.dto.bossPhaseTransitions);
		}

		for (size_t i = 0; i < profiles.size(); ++i)
		{
			profiles[i].combatActions = std::span<const WeightedActionEntry>(
				outDefs.combatActionStorage[i].data(),
				outDefs.combatActionStorage[i].size());
			profiles[i].idleActions = std::span<const WeightedActionEntry>(
				outDefs.idleActionStorage[i].data(),
				outDefs.idleActionStorage[i].size());
			profiles[i].bossPattern.phaseTransitions =
				std::span<const AIBossPhaseTransitionDef>(
					outDefs.bossPhaseTransitionStorage[i].data(),
					outDefs.bossPhaseTransitionStorage[i].size());
		}

		if (!ValidateLoadedProfiles(
				std::span<const AIBehaviorProfileDef>(
					profiles.data(),
					profiles.size()),
				outError))
		{
			return false;
		}

		std::string registryError;
		if (!outDefs.profiles.Build(std::move(profiles), &registryError))
		{
			outError = registryError;
			return false;
		}

		return true;
	}
}

AIBehaviorDefLoadResult LoadAIBehaviorProfileDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AIBehaviorDefinitionSet& outDefs)
{
	std::vector<DefJsonDocument> documents;
	AIBehaviorDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "AI behavior");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<AIBehaviorProfileDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(
				documents.data(),
				documents.size()),
			AppendAIBehaviorDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	if (!BuildAIBehaviorDefinitionSet(
			std::span<const DefParsedDto<AIBehaviorProfileDto>>(
				dtos.data(),
				dtos.size()),
			outDefs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	result.succeeded = true;
	result.loadedCount = outDefs.profiles.Size();
	result.error.clear();
	return result;
}
