#include "pch.h"
#include "CharacterDef.h"

#include "DefEnumString.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"
#include "json.hpp"

#include <vector>

using json = nlohmann::json;

namespace
{
	struct CharacterDefTraits
	{
		static CharacterId GetId(const CharacterDef& def) noexcept
		{
			return def.id;
		}
	};

	using CharacterDefRegistry =
		DefRegistry<CharacterDef, CharacterId, CharacterDefTraits>;

	CharacterDefRegistry& GetCharacterDefRegistry() noexcept
	{
		static CharacterDefRegistry registry;
		return registry;
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

	bool ReadCharacterActionProfileId(
		const json& node,
		const char* field,
		CharacterActionProfileId& outValue,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!ParseCharacterActionProfileIdString(text, outValue))
		{
			outError = "Unknown character action profile id: " + text;
			return false;
		}

		return true;
	}

	bool ParseProfile(
		const json& node,
		CharacterProfileDef& outProfile,
		std::string& outError)
	{
		return ReadEnum(
			node,
			"faction",
			outProfile.faction,
			"character faction",
			outError);
	}

	bool ParseStat(
		const json& node,
		CharacterStatDef& outStat,
		std::string& outError)
	{
		return
			ReadRequiredNumber(node, "maxHp", outStat.maxHp, outError) &&
			ReadRequiredNumber(node, "maxStamina", outStat.maxStamina, outError) &&
			ReadRequiredNumber(node, "maxPoise", outStat.maxPoise, outError) &&
			ReadRequiredNumber(node, "attackPower", outStat.attackPower, outError) &&
			ReadRequiredNumber(node, "defense", outStat.defense, outError) &&
			ReadRequiredNumber(node, "moveSpeed", outStat.moveSpeed, outError) &&
			ReadRequiredNumber(node, "attackSpeed", outStat.attackSpeed, outError);
	}

	bool ParseBodyCollision(
		const json& node,
		CharacterBodyCollisionDef& outBody,
		std::string& outError)
	{
		if (!ReadEnum(
			node,
			"pushability",
			outBody.pushability,
			"body pushability",
			outError))
		{
			return false;
		}

		return
			ReadRequiredNumber(node, "footprintRadiusXZ", outBody.footprintRadiusXZ, outError) &&
			ReadRequiredNumber(node, "bodyHeight", outBody.bodyHeight, outError) &&
			ReadRequiredBool(node, "blocksBodyOverlap", outBody.blocksBodyOverlap, outError) &&
			ReadRequiredBool(node, "useNavMeshConstraint", outBody.useNavMeshConstraint, outError) &&
			ReadRequiredNumber(node, "overlapYieldWeight", outBody.overlapYieldWeight, outError) &&
			ReadRequiredNumber(
				node,
				"maxOverlapCorrectionPerFrameXZ",
				outBody.maxOverlapCorrectionPerFrameXZ,
				outError);
	}

	bool ParseFeatures(
		const json& node,
		CharacterFeatureFlags& outFeatures,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Character features must be an array.";
			return false;
		}

		outFeatures = CharacterFeatureFlags::None;
		for (const json& featureNode : node)
		{
			if (!featureNode.is_string())
			{
				outError = "Character feature entry must be a string.";
				return false;
			}

			const std::string text = featureNode.get<std::string>();
			CharacterFeatureFlags flag{};
			if (!ParseDefString(text, flag))
			{
				outError = "Unknown character feature flag: " + text;
				return false;
			}

			outFeatures = outFeatures | flag;
		}

		return true;
	}

	bool ParseAI(
		const json& node,
		std::optional<CharacterAIDef>& outAI,
		std::string& outError)
	{
		if (node.is_null())
		{
			outAI = std::nullopt;
			return true;
		}

		CharacterAIDef ai{};
		if (!ReadEnum(node, "aiType", ai.aiType, "AI archetype", outError))
			return false;

		if (node.contains("aiTuningId") && !node.at("aiTuningId").is_null())
		{
			if (!node.at("aiTuningId").is_string())
			{
				outError = "aiTuningId must be a string or null.";
				return false;
			}

			const std::string text = node.at("aiTuningId").get<std::string>();
			AITuningId tuningId{};
			if (!ParseAITuningIdString(text, tuningId))
			{
				outError = "Unknown AI tuning id: " + text;
				return false;
			}

			ai.aiTuningId = tuningId;
		}
		else
		{
			ai.aiTuningId = std::nullopt;
		}

		outAI = ai;
		return true;
	}

	bool ParseAction(
		const json& node,
		std::optional<CharacterActionDefRef>& outAction,
		std::string& outError)
	{
		if (node.is_null())
		{
			outAction = std::nullopt;
			return true;
		}

		CharacterActionProfileId profileId{};
		if (!ReadCharacterActionProfileId(
			node,
			"actionProfileId",
			profileId,
			outError))
		{
			return false;
		}

		outAction = CharacterActionDefRef{ .actionProfileId = profileId };
		return true;
	}

	bool ParseCharacterDocument(
		const json& root,
		CharacterDef& outDef,
		std::string& outError)
	{
		if (!ReadEnum(root, "id", outDef.id, "character id", outError))
			return false;

		if (!ReadRequiredString(root, "name", outDef.name, outError))
			return false;

		if (!root.contains("profile") ||
			!ParseProfile(root.at("profile"), outDef.profile, outError))
		{
			return false;
		}

		if (!root.contains("stat") ||
			!ParseStat(root.at("stat"), outDef.stat, outError))
		{
			return false;
		}

		if (!root.contains("bodyCollision") ||
			!ParseBodyCollision(
				root.at("bodyCollision"),
				outDef.bodyCollision,
				outError))
		{
			return false;
		}

		if (!ReadEnum(root, "role", outDef.role, "character role", outError))
			return false;

		if (!root.contains("features") ||
			!ParseFeatures(root.at("features"), outDef.features, outError))
		{
			return false;
		}

		if (root.contains("ai") &&
			!ParseAI(root.at("ai"), outDef.ai, outError))
		{
			return false;
		}

		if (root.contains("action") &&
			!ParseAction(root.at("action"), outDef.action, outError))
		{
			return false;
		}

		return true;
	}

	bool ValidateCharacterDefs(
		std::span<const CharacterDef> defs,
		std::string& outError)
	{
		for (const CharacterDef& def : defs)
		{
			if (def.id == CharacterId::None)
			{
				outError = "Character id must not be None.";
				return false;
			}

			if (def.name.empty())
			{
				outError = "Character name must not be empty.";
				return false;
			}

			if (def.IsAIControlled() && !def.ai.has_value())
			{
				outError = "AIControlled character requires ai.";
				return false;
			}
		}

		return true;
	}
}

const CharacterDef* FindCharacterDef(CharacterId id) noexcept
{
	return GetCharacterDefRegistry().Find(id);
}

const CharacterDef& GetCharacterDef(CharacterId id)
{
	return GetCharacterDefRegistry().Get(id);
}

std::span<const CharacterDef> GetCharacterDefs() noexcept
{
	return GetCharacterDefRegistry().GetAll();
}

CharacterDefLoadResult LoadCharacterDefsFromJsonDirectory(
	const std::filesystem::path& directory)
{
	std::vector<DefJsonDocument> documents;
	CharacterDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "Character");
	if (!result.succeeded)
		return result;

	std::vector<CharacterDef> defs(documents.size());
	for (size_t i = 0; i < documents.size(); ++i)
	{
		try
		{
			if (!ParseCharacterDocument(
				documents[i].root,
				defs[i],
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
				": Invalid character json field: " + std::string(ex.what());
			result.succeeded = false;
			return result;
		}
	}

	if (!ValidateCharacterDefs(defs, result.error))
	{
		result.succeeded = false;
		return result;
	}

	CharacterDefRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}

	CharacterDefRegistry& activeRegistry = GetCharacterDefRegistry();
	activeRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = activeRegistry.Size();
	result.error.clear();
	return result;
}
