#include "pch.h"
#include "AIBehaviorDef.h"

#include "DefRegistry.h"
#include "json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
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

	bool ReadFileText(
		const std::filesystem::path& path,
		std::string& outText,
		std::string& outError)
	{
		std::ifstream input(path);
		if (!input.is_open())
		{
			outError = "Failed to open AI behavior json file: " + path.string();
			return false;
		}

		std::stringstream buffer;
		buffer << input.rdbuf();
		outText = buffer.str();
		return true;
	}

	bool ReadString(
		const json& node,
		const char* field,
		std::string& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || !node.at(field).is_string())
		{
			outError = std::string("Missing or invalid string field: ") + field;
			return false;
		}

		outValue = node.at(field).get<std::string>();
		return true;
	}

	template<typename TValue>
	bool ReadOptionalNumber(
		const json& node,
		const char* field,
		TValue& outValue,
		std::string& outError)
	{
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_number())
		{
			outError = std::string("Invalid numeric field: ") + field;
			return false;
		}

		outValue = node.at(field).get<TValue>();
		return true;
	}

	bool ParseAITuningId(
		std::string_view text,
		AITuningId& outValue) noexcept
	{
		if (text == "None") { outValue = AITuningIds::None; return true; }
		if (text == "Imp") { outValue = AITuningIds::Imp; return true; }
		if (text == "DemonStriker") { outValue = AITuningIds::DemonStriker; return true; }
		if (text == "DemonExecutioner") { outValue = AITuningIds::DemonExecutioner; return true; }
		if (text == "FinalBoss") { outValue = AITuningIds::FinalBoss; return true; }
		return false;
	}

	bool ParseAIArchetype(
		std::string_view text,
		AIArchetype& outValue) noexcept
	{
		if (text == "None") { outValue = AIArchetype::None; return true; }
		if (text == "NormalMonster") { outValue = AIArchetype::NormalMonster; return true; }
		if (text == "FinalBossMonster") { outValue = AIArchetype::FinalBossMonster; return true; }
		return false;
	}

	bool ParseMovementPolicyKind(
		std::string_view text,
		AIMovementPolicyKind& outValue) noexcept
	{
		if (text == "None") { outValue = AIMovementPolicyKind::None; return true; }
		if (text == "Normal") { outValue = AIMovementPolicyKind::Normal; return true; }
		return false;
	}

	bool ParseCombatActionPolicyKind(
		std::string_view text,
		AICombatActionPolicyKind& outValue) noexcept
	{
		if (text == "None") { outValue = AICombatActionPolicyKind::None; return true; }
		if (text == "Imp") { outValue = AICombatActionPolicyKind::Imp; return true; }
		if (text == "Weighted") { outValue = AICombatActionPolicyKind::Weighted; return true; }
		return false;
	}

	bool ParseIdleActionPolicyKind(
		std::string_view text,
		AIIdleActionPolicyKind& outValue) noexcept
	{
		if (text == "None") { outValue = AIIdleActionPolicyKind::None; return true; }
		if (text == "Weighted") { outValue = AIIdleActionPolicyKind::Weighted; return true; }
		return false;
	}

	bool ParseReactionPolicyKind(
		std::string_view text,
		AIReactionPolicyKind& outValue) noexcept
	{
		if (text == "None") { outValue = AIReactionPolicyKind::None; return true; }
		if (text == "Normal") { outValue = AIReactionPolicyKind::Normal; return true; }
		return false;
	}

	bool ParseActionId(
		std::string_view text,
		ActionId& outValue) noexcept
	{
		static const std::unordered_map<std::string_view, ActionId> kActionIds =
		{
			{ "None", ActionId::None },
			{ "Imp_melee1", ActionId::Imp_melee1 },
			{ "Imp_melee2", ActionId::Imp_melee2 },
			{ "Imp_melee3", ActionId::Imp_melee3 },
			{ "Imp_melee4", ActionId::Imp_melee4 },
			{ "Imp_melee5", ActionId::Imp_melee5 },
			{ "Imp_Jump", ActionId::Imp_Jump },
			{ "DemonStriker_Melee_1", ActionId::DemonStriker_Melee_1 },
			{ "DemonStriker_Melee_2", ActionId::DemonStriker_Melee_2 },
			{ "DemonStriker_Melee_3", ActionId::DemonStriker_Melee_3 },
			{ "DemonStriker_Melee_4", ActionId::DemonStriker_Melee_4 },
			{ "DemonStriker_Jump_1", ActionId::DemonStriker_Jump_1 },
			{ "DemonStriker_Jump_2", ActionId::DemonStriker_Jump_2 },
			{ "DemonExecutioner_Melee_1", ActionId::DemonExecutioner_Melee_1 },
			{ "DemonExecutioner_Melee_2", ActionId::DemonExecutioner_Melee_2 },
			{ "DemonExecutioner_Melee_3", ActionId::DemonExecutioner_Melee_3 },
			{ "DemonExecutioner_Melee_4", ActionId::DemonExecutioner_Melee_4 },
			{ "DemonExecutioner_Melee_5", ActionId::DemonExecutioner_Melee_5 },
			{ "DemonExecutioner_Melee_6", ActionId::DemonExecutioner_Melee_6 },
			{ "DemonExecutioner_Jump_1", ActionId::DemonExecutioner_Jump_1 },
			{ "DemonExecutioner_Jump_2", ActionId::DemonExecutioner_Jump_2 }
		};

		const auto it = kActionIds.find(text);
		if (it == kActionIds.end())
			return false;

		outValue = it->second;
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
			std::string actionText;
			if (!ReadString(actionNode, "actionId", actionText, outError))
				return false;

			WeightedActionEntry entry{};
			if (!ParseActionId(actionText, entry.actionId))
			{
				outError = "Unknown ActionId: " + actionText;
				return false;
			}

			if (!ReadOptionalNumber(actionNode, "weight", entry.weight, outError))
				return false;

			outActions.push_back(entry);
		}

		return true;
	}

	bool ReadEnumString(
		const json& node,
		const char* field,
		std::string& outText,
		std::string& outError)
	{
		return ReadString(node, field, outText, outError);
	}

	bool ParseProfileDocument(
		const json& root,
		AIBehaviorProfileDef& outProfile,
		std::vector<WeightedActionEntry>& outCombatActions,
		std::vector<WeightedActionEntry>& outIdleActions,
		std::string& outError)
	{
		std::string text;
		if (!ReadEnumString(root, "id", text, outError))
			return false;
		if (!ParseAITuningId(text, outProfile.id))
		{
			outError = "Unknown AI tuning id: " + text;
			return false;
		}

		if (!ReadEnumString(root, "aiType", text, outError))
			return false;
		if (!ParseAIArchetype(text, outProfile.aiType))
		{
			outError = "Unknown AI archetype: " + text;
			return false;
		}

		if (root.contains("perceptionTuning") &&
			!ParsePerceptionTuning(root.at("perceptionTuning"), outProfile.perceptionTuning, outError))
		{
			return false;
		}

		if (root.contains("decisionTuning") &&
			!ParseDecisionTuning(root.at("decisionTuning"), outProfile.decisionTuning, outError))
		{
			return false;
		}

		if (!ReadEnumString(root, "movementPolicyKind", text, outError))
			return false;
		if (!ParseMovementPolicyKind(text, outProfile.movementPolicyKind))
		{
			outError = "Unknown AI movement policy kind: " + text;
			return false;
		}

		if (!ReadEnumString(root, "combatActionPolicyKind", text, outError))
			return false;
		if (!ParseCombatActionPolicyKind(text, outProfile.combatActionPolicyKind))
		{
			outError = "Unknown AI combat action policy kind: " + text;
			return false;
		}

		if (!ReadEnumString(root, "idleActionPolicyKind", text, outError))
			return false;
		if (!ParseIdleActionPolicyKind(text, outProfile.idleActionPolicyKind))
		{
			outError = "Unknown AI idle action policy kind: " + text;
			return false;
		}

		if (!ReadEnumString(root, "reactionPolicyKind", text, outError))
			return false;
		if (!ParseReactionPolicyKind(text, outProfile.reactionPolicyKind))
		{
			outError = "Unknown AI reaction policy kind: " + text;
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

	bool LoadProfileFile(
		const std::filesystem::path& path,
		AIBehaviorProfileDef& outProfile,
		std::vector<WeightedActionEntry>& outCombatActions,
		std::vector<WeightedActionEntry>& outIdleActions,
		std::string& outError)
	{
		std::string jsonText;
		if (!ReadFileText(path, jsonText, outError))
			return false;

		json root;
		try
		{
			root = json::parse(jsonText);
		}
		catch (const std::exception& ex)
		{
			outError = "Failed to parse AI behavior json: " + std::string(ex.what());
			return false;
		}

		try
		{
			return ParseProfileDocument(
				root,
				outProfile,
				outCombatActions,
				outIdleActions,
				outError);
		}
		catch (const std::exception& ex)
		{
			outError = "Invalid AI behavior json field: " + std::string(ex.what());
			return false;
		}
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
	AIBehaviorDefLoadResult result{};
	GetAIBehaviorDefs() = {};

	std::error_code ec;
	if (!std::filesystem::exists(directory, ec) ||
		!std::filesystem::is_directory(directory, ec))
	{
		result.error = "AI behavior json directory was not found: " + directory.string();
		return result;
	}

	std::vector<std::filesystem::path> files;
	for (const std::filesystem::directory_entry& entry :
		std::filesystem::directory_iterator(directory, ec))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".json")
			files.push_back(entry.path());
	}

	std::sort(files.begin(), files.end());
	if (files.empty())
	{
		result.error = "AI behavior json directory has no json files: " + directory.string();
		return result;
	}

	AIBehaviorDefinitionSet loaded{};
	std::vector<AIBehaviorProfileDef> profiles(files.size());
	loaded.combatActionStorage.resize(files.size());
	loaded.idleActionStorage.resize(files.size());

	for (size_t i = 0; i < files.size(); ++i)
	{
		if (!LoadProfileFile(
			files[i],
			profiles[i],
			loaded.combatActionStorage[i],
			loaded.idleActionStorage[i],
			result.error))
		{
			result.error = files[i].string() + ": " + result.error;
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
		return result;

	std::string registryError;
	if (!loaded.profiles.Build(std::move(profiles), &registryError))
	{
		result.error = registryError;
		return result;
	}

	AIBehaviorDefinitionSet& activeDefs = GetAIBehaviorDefs();
	activeDefs = std::move(loaded);
	result.succeeded = true;
	result.loadedCount = activeDefs.profiles.Size();
	return result;
}
