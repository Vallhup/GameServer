#include "pch.h"
#include "AIBehaviorDef.h"

#include "DefEnumString.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"
#include "json.hpp"

#include <vector>

using json = nlohmann::json;

namespace
{
	struct AIBehaviorDefinitionSet
	{
		struct ProfileKey
		{
			AIArchetype aiType{ AIArchetype::None };
			AITuningId aiTuningId{ AITuningIds::None };

			bool operator==(const ProfileKey& other) const noexcept
			{
				return aiType == other.aiType &&
					aiTuningId == other.aiTuningId;
			}
		};

		struct ProfileKeyHash
		{
			size_t operator()(ProfileKey key) const noexcept
			{
				const size_t aiTypeHash =
					DefRegistryIdHash<AIArchetype>{}(key.aiType);
				const size_t tuningHash =
					DefRegistryIdHash<AITuningId>{}(key.aiTuningId);
				return aiTypeHash ^ (tuningHash + 0x9e3779b9u +
					(aiTypeHash << 6u) + (aiTypeHash >> 2u));
			}
		};

		struct ProfileTraits
		{
			static ProfileKey GetId(const AIBehaviorProfileDef& def) noexcept
			{
				return ProfileKey
				{
					.aiType = def.aiType,
					.aiTuningId = def.id
				};
			}
		};

		using ProfileRegistry = DefRegistry<
			AIBehaviorProfileDef,
			ProfileKey,
			ProfileTraits,
			ProfileKeyHash>;

		ProfileRegistry profiles;
		std::vector<std::vector<WeightedActionEntry>> combatActionStorage;
		std::vector<std::vector<WeightedActionEntry>> idleActionStorage;
	};

	AIBehaviorDefinitionSet& GetAIBehaviorDefs() noexcept
	{
		static AIBehaviorDefinitionSet defs;
		return defs;
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

	bool ReadAITuningId(
		const json& node,
		const char* field,
		AITuningId& outValue,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!ParseAITuningIdString(text, outValue))
		{
			outError = "Unknown AI tuning id: " + text;
			return false;
		}

		return true;
	}

	bool ParsePerceptionTuning(
		const json& node,
		AIPerceptionTuningComp& outTuning,
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
		AIDecisionTuningComp& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "decisionInterval", outTuning.decisionInterval, outError) &&
			ReadOptionalNumber(node, "attackCooldown", outTuning.attackCooldown, outError) &&
			ReadOptionalNumber(node, "reactDuration", outTuning.reactDuration, outError);
	}

	bool ParseWeightedActions(
		const json& node,
		std::vector<WeightedActionEntry>& outActions,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Weighted action list must be an array.";
			return false;
		}

		outActions.clear();
		outActions.reserve(node.size());
		for (const json& actionNode : node)
		{
			WeightedActionEntry entry{};
			if (!ReadEnum(
				actionNode,
				"actionId",
				entry.actionId,
				"ActionId",
				outError))
			{
				return false;
			}

			if (!ReadOptionalNumber(actionNode, "weight", entry.weight, outError))
				return false;

			outActions.push_back(entry);
		}

		return true;
	}

	bool ParseProfileDocument(
		const json& root,
		AIBehaviorProfileDef& outProfile,
		std::vector<WeightedActionEntry>& outCombatActions,
		std::vector<WeightedActionEntry>& outIdleActions,
		std::string& outError)
	{
		if (!ReadAITuningId(root, "id", outProfile.id, outError) ||
			!ReadEnum(root, "aiType", outProfile.aiType, "AI archetype", outError))
		{
			return false;
		}

		if (root.contains("perceptionTuning") &&
			!ParsePerceptionTuning(
				root.at("perceptionTuning"),
				outProfile.perceptionTuning,
				outError))
		{
			return false;
		}

		if (root.contains("decisionTuning") &&
			!ParseDecisionTuning(
				root.at("decisionTuning"),
				outProfile.decisionTuning,
				outError))
		{
			return false;
		}

		if (!ReadEnum(
				root,
				"movementPolicyKind",
				outProfile.movementPolicyKind,
				"AI movement policy kind",
				outError) ||
			!ReadEnum(
				root,
				"combatActionPolicyKind",
				outProfile.combatActionPolicyKind,
				"AI combat action policy kind",
				outError) ||
			!ReadEnum(
				root,
				"idleActionPolicyKind",
				outProfile.idleActionPolicyKind,
				"AI idle action policy kind",
				outError) ||
			!ReadEnum(
				root,
				"reactionPolicyKind",
				outProfile.reactionPolicyKind,
				"AI reaction policy kind",
				outError))
		{
			return false;
		}

		if (root.contains("combatActions") &&
			!ParseWeightedActions(root.at("combatActions"), outCombatActions, outError))
		{
			return false;
		}

		if (root.contains("idleActions") &&
			!ParseWeightedActions(root.at("idleActions"), outIdleActions, outError))
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
			if (profile.id == AITuningIds::None)
			{
				outError = "AI behavior profile id must not be None.";
				return false;
			}

			for (size_t j = i + 1; j < profiles.size(); ++j)
			{
				const AIBehaviorProfileDef& other = profiles[j];
				if (profile.id == other.id && profile.aiType == other.aiType)
				{
					outError = "Duplicate AI behavior profile id and archetype.";
					return false;
				}
			}

			if (profile.combatActionPolicyKind == AICombatActionPolicyKind::Weighted &&
				profile.combatActions.empty())
			{
				outError = "Weighted AI combat action policy requires combatActions.";
				return false;
			}

			if (profile.idleActionPolicyKind == AIIdleActionPolicyKind::Weighted &&
				profile.idleActions.empty())
			{
				outError = "Weighted AI idle action policy requires idleActions.";
				return false;
			}
		}

		return true;
	}
}

const AIBehaviorProfileDef* FindAIBehaviorProfileDef(
	AIArchetype aiType,
	AITuningId aiTuningId) noexcept
{
	const AIBehaviorDefinitionSet::ProfileKey key
	{
		.aiType = aiType,
		.aiTuningId = aiTuningId
	};

	return GetAIBehaviorDefs().profiles.Find(key);
}

std::span<const AIBehaviorProfileDef> GetAIBehaviorProfileDefs() noexcept
{
	return GetAIBehaviorDefs().profiles.GetAll();
}

AIBehaviorDefLoadResult LoadAIBehaviorProfileDefsFromJsonDirectory(
	const std::filesystem::path& directory)
{
	std::vector<DefJsonDocument> documents;
	AIBehaviorDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "AI behavior");
	if (!result.succeeded)
		return result;

	AIBehaviorDefinitionSet loaded{};
	std::vector<AIBehaviorProfileDef> profiles(documents.size());
	loaded.combatActionStorage.resize(documents.size());
	loaded.idleActionStorage.resize(documents.size());

	for (size_t i = 0; i < documents.size(); ++i)
	{
		try
		{
			if (!ParseProfileDocument(
				documents[i].root,
				profiles[i],
				loaded.combatActionStorage[i],
				loaded.idleActionStorage[i],
				result.error))
			{
				result.error = documents[i].path.string() + ": " + result.error;
				result.succeeded = false;
				return result;
			}
		}
		catch (const std::exception& ex)
		{
			result.error = documents[i].path.string() +
				": Invalid AI behavior json field: " + std::string(ex.what());
			result.succeeded = false;
			return result;
		}
	}

	for (size_t i = 0; i < profiles.size(); ++i)
	{
		profiles[i].combatActions =
			std::span<const WeightedActionEntry>(loaded.combatActionStorage[i]);
		profiles[i].idleActions =
			std::span<const WeightedActionEntry>(loaded.idleActionStorage[i]);
	}

	if (!ValidateLoadedProfiles(profiles, result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::string registryError;
	if (!loaded.profiles.Build(std::move(profiles), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}

	AIBehaviorDefinitionSet& activeDefs = GetAIBehaviorDefs();
	activeDefs = std::move(loaded);
	result.succeeded = true;
	result.loadedCount = activeDefs.profiles.Size();
	result.error.clear();
	return result;
}
