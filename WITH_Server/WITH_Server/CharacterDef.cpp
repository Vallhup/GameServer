#include "pch.h"
#include "CharacterDef.h"

#include "DefCompilePipeline.h"
#include "DefEnumString.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"
#include "json.hpp"

#include <vector>

using json = nlohmann::json;

namespace
{
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
		if (!ReadRequiredNumber(node, "maxHp", outStat.maxHp, outError) ||
			!ReadRequiredNumber(node, "maxStamina", outStat.maxStamina, outError) ||
			!ReadRequiredNumber(node, "maxPoise", outStat.maxPoise, outError) ||
			!ReadRequiredNumber(node, "attackPower", outStat.attackPower, outError) ||
			!ReadRequiredNumber(node, "defense", outStat.defense, outError) ||
			!ReadRequiredNumber(node, "moveSpeed", outStat.moveSpeed, outError) ||
			!ReadRequiredNumber(node, "attackSpeed", outStat.attackSpeed, outError))
		{
			return false;
		}

		outStat.walkSpeed.reset();
		if (node.contains("walkSpeed"))
		{
			float walkSpeed = 0.0f;
			if (!ReadOptionalNumber(node, "walkSpeed", walkSpeed, outError))
			{
				return false;
			}
			outStat.walkSpeed = walkSpeed;
		}

		return true;
	}

	bool ParseAttributes(
		const json& node,
		std::vector<CharacterAttributeInitialValueDef>& outAttributes,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Character attributes must be an array.";
			return false;
		}

		outAttributes.clear();
		outAttributes.reserve(node.size());
		for (const json& attributeNode : node)
		{
			CharacterAttributeInitialValueDef attribute{};
			if (!ReadRequiredString(
					attributeNode,
					"attributeKey",
					attribute.attributeKey,
					outError) ||
				!ReadRequiredNumber(
					attributeNode,
					"baseValue",
					attribute.baseValue,
					outError) ||
				!ReadRequiredNumber(
					attributeNode,
					"currentValue",
					attribute.currentValue,
					outError))
			{
				return false;
			}

			outAttributes.push_back(std::move(attribute));
		}

		return true;
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

		if (node.contains("aiProfileKey") && !node.at("aiProfileKey").is_null())
		{
			if (!node.at("aiProfileKey").is_string())
			{
				outError = "aiProfileKey must be a string or null.";
				return false;
			}

			ai.aiProfileKey = node.at("aiProfileKey").get<std::string>();
			ai.aiProfileId = HashDefKey(ai.aiProfileKey);
			if (ai.aiProfileId == InvalidAIBehaviorProfileId)
			{
				outError = "Invalid AI profile key: " + ai.aiProfileKey;
				return false;
			}
		}
		else
		{
			ai.aiProfileKey.clear();
			ai.aiProfileId = InvalidAIBehaviorProfileId;
		}

		outAI = ai;
		return true;
	}

	bool ParseKillBuffGrants(
		const json& node,
		std::vector<KillBuffGrantDef>& outGrants,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "killBuffGrants must be an array.";
			return false;
		}

		for (const json& grantNode : node)
		{
			KillBuffGrantDef grant{};
			if (!ReadRequiredString(
					grantNode,
					"effectKey",
					grant.buffEffectKey,
					outError))
			{
				return false;
			}
			if (!ReadRequiredNumber(
					grantNode,
					"grantProbability",
					grant.grantProbability,
					outError))
			{
				return false;
			}

			outGrants.push_back(std::move(grant));
		}

		return true;
	}

	bool ParseAbilitySetKey(
		const json& node,
		std::optional<std::string>& outAbilitySetKey,
		std::string& outError)
	{
		if (node.is_null())
		{
			outAbilitySetKey = std::nullopt;
			return true;
		}

		if (!node.is_string())
		{
			outError = "abilitySetKey must be a string or null.";
			return false;
		}

		outAbilitySetKey = node.get<std::string>();
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

		if (root.contains("attributes") &&
			!ParseAttributes(root.at("attributes"), outDef.attributes, outError))
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

		if (root.contains("abilitySetKey") &&
			!ParseAbilitySetKey(
				root.at("abilitySetKey"),
				outDef.abilitySetKey,
				outError))
		{
			return false;
		}

		if (root.contains("killBuffGrants") &&
			!ParseKillBuffGrants(
				root.at("killBuffGrants"),
				outDef.killBuffGrants,
				outError))
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

			if (def.stat.walkSpeed.has_value() &&
				def.stat.walkSpeed.value() < 0.0f)
			{
				outError = "Character walk speed must not be negative.";
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

	bool AppendCharacterDocumentDtos(
		const json& root,
		std::vector<CharacterDef>& outDtos,
		std::string& outError)
	{
		CharacterDef def{};
		if (!ParseCharacterDocument(root, def, outError))
			return false;

		outDtos.push_back(std::move(def));
		return true;
	}

	bool CompileCharacterDef(
		const CharacterDef& dto,
		CharacterDef& outDef,
		std::string& outError)
	{
		(void)outError;
		outDef = dto;
		return true;
	}
}

CharacterDefLoadResult LoadCharacterDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	CharacterDefRegistry& outRegistry)
{
	return LoadCompiledDefsFromJsonDirectory<CharacterDef, CharacterDef>(
		directory,
		"Character",
		AppendCharacterDocumentDtos,
		CompileCharacterDef,
		ValidateCharacterDefs,
		outRegistry);
}
