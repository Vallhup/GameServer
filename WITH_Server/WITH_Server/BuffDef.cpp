#include "pch.h"
#include "BuffDef.h"

#include "DefEnumString.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"

#include <vector>

using json = nlohmann::json;

namespace
{
	struct BuffDefTraits
	{
		static BuffId GetId(const BuffDef& def) noexcept
		{
			return def.profile.id;
		}
	};

	using BuffDefRegistry = DefRegistry<BuffDef, BuffId, BuffDefTraits>;

	BuffDefRegistry& GetBuffDefRegistry() noexcept
	{
		static BuffDefRegistry registry;
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

	bool ParseProfile(
		const json& node,
		BuffProfileDef& outProfile,
		std::string& outError)
	{
		if (!ReadEnum(node, "id", outProfile.id, "BuffId", outError) ||
			!ReadRequiredString(node, "name", outProfile.name, outError) ||
			!ReadEnum(node, "kind", outProfile.kind, "BuffKind", outError) ||
			!ReadEnum(node, "familyId", outProfile.familyId, "BuffFamilyId", outError) ||
			!ReadEnum(node, "tier", outProfile.tier, "BuffTier", outError))
		{
			return false;
		}

		BuffFamilyId groupId{};
		if (!ReadEnum(node, "groupId", groupId, "BuffFamilyId", outError))
			return false;

		outProfile.groupId = groupId;

		return ReadEnum(
			node,
			"familyStackPolicy",
			outProfile.familyStackPolicy,
			"BuffFamilyStackPolicy",
			outError);
	}

	bool ParseLifetime(
		const json& node,
		BuffLifetimeDef& outLifetime,
		std::string& outError)
	{
		return
			ReadEnum(node, "durationPolicy", outLifetime.durationPolicy, "DurationPolicy", outError) &&
			ReadRequiredNumber(node, "defaultDurationSec", outLifetime.defaultDurationSec, outError) &&
			ReadNullableNumber(node, "tickIntervalSec", outLifetime.tickIntervalSec, outError) &&
			ReadRequiredNumber(node, "maxStackCount", outLifetime.maxStackCount, outError) &&
			ReadEnum(node, "overlappingPolicy", outLifetime.overlappingPolicy, "OverlappingPolicy", outError) &&
			ReadEnum(node, "reapplyPolicy", outLifetime.reapplyPolicy, "ReapplyPolicy", outError);
	}

	bool ParseModifierCondition(
		const json& node,
		std::optional<BuffModifierCondition>& outCondition,
		std::string& outError)
	{
		if (node.is_null())
		{
			outCondition = std::nullopt;
			return true;
		}

		BuffModifierCondition condition{};
		if (!ReadEnum(node, "type", condition.type, "BuffModifierConditionType", outError) ||
			!ReadEnum(node, "stateFlag", condition.stateFlag, "GameplayStateFlag", outError))
		{
			return false;
		}

		outCondition = condition;
		return true;
	}

	bool ParseEffect(
		const json& node,
		BuffEffectDef& outEffect,
		std::string& outError)
	{
		if (!ReadEnum(node, "type", outEffect.type, "BuffEffectType", outError) ||
			!ReadEnum(node, "timing", outEffect.timing, "BuffModifierTiming", outError))
		{
			return false;
		}

		if (node.contains("stat") && !node.at("stat").is_null())
		{
			const json& statNode = node.at("stat");
			BuffStatEffectDef stat{};
			if (!ReadEnum(statNode, "statType", stat.statType, "StatType", outError) ||
				!ReadRequiredNumber(statNode, "value", stat.value, outError))
			{
				return false;
			}
			outEffect.stat = stat;
		}

		if (node.contains("stateFlag") && !node.at("stateFlag").is_null())
		{
			const json& stateFlagNode = node.at("stateFlag");
			BuffStateFlagEffectDef stateFlag{};
			if (!ReadEnum(
				stateFlagNode,
				"flag",
				stateFlag.flag,
				"GameplayStateFlag",
				outError))
			{
				return false;
			}
			outEffect.stateFlag = stateFlag;
		}

		if (node.contains("periodic") && !node.at("periodic").is_null())
		{
			outError = "Buff periodic effect json is not supported yet.";
			return false;
		}

		if (node.contains("condition") &&
			!ParseModifierCondition(node.at("condition"), outEffect.condition, outError))
		{
			return false;
		}

		return true;
	}

	bool ParseApplyRequirement(
		const json& node,
		BuffApplyRequirementDef& outRequirement,
		std::string& outError)
	{
		if (!ReadEnum(
			node,
			"type",
			outRequirement.type,
			"BuffApplyRequirementType",
			outError))
		{
			return false;
		}

		const json* operand = node.contains("operand") ? &node.at("operand") : nullptr;
		if (operand == nullptr || !operand->is_object())
		{
			outError = "Buff apply requirement requires operand object.";
			return false;
		}

		return
			ReadNullableNumber(*operand, "scalarCondition", outRequirement.operand.scalarCondition, outError) &&
			ReadNullableEnum(*operand, "stateFlagCondition", outRequirement.operand.stateFlagCondition, "GameplayStateFlag", outError) &&
			ReadNullableEnum(*operand, "buffFamilyIdCondition", outRequirement.operand.buffFamilyIdCondition, "BuffFamilyId", outError) &&
			ReadNullableEnum(*operand, "factionCondition", outRequirement.operand.factionCondition, "Faction", outError);
	}

	bool ParseRemoveRule(
		const json& node,
		BuffRemoveRuleDef& outRule,
		std::string& outError)
	{
		if (!ReadEnum(node, "type", outRule.type, "BuffRemoveRuleType", outError))
			return false;

		const json* operand = node.contains("operand") ? &node.at("operand") : nullptr;
		if (operand == nullptr || !operand->is_object())
		{
			outError = "Buff remove rule requires operand object.";
			return false;
		}

		return
			ReadNullableNumber(*operand, "scalarCondition", outRule.operand.scalarCondition, outError) &&
			ReadNullableEnum(*operand, "stateFlagCondition", outRule.operand.stateFlagCondition, "GameplayStateFlag", outError) &&
			ReadNullableEnum(*operand, "actionIdCondition", outRule.operand.actionIdCondition, "ActionId", outError) &&
			ReadNullableEnum(*operand, "actionKindCondition", outRule.operand.actionKindCondition, "ActionKind", outError) &&
			ReadNullableEnum(*operand, "buffFamilyIdCondition", outRule.operand.buffFamilyIdCondition, "BuffFamilyId", outError) &&
			ReadNullableEnum(*operand, "counterEventCondition", outRule.operand.counterEventCondition, "BuffCounterEventType", outError);
	}

	template<typename TDef, typename TParser>
	bool ParseArray(
		const json& node,
		const char* field,
		std::vector<TDef>& outDefs,
		TParser parser,
		std::string& outError)
	{
		if (!node.contains(field) || !node.at(field).is_array())
		{
			outError = std::string("Missing or invalid array field: ") + field;
			return false;
		}

		const json& arrayNode = node.at(field);
		outDefs.clear();
		outDefs.reserve(arrayNode.size());
		for (const json& item : arrayNode)
		{
			TDef def{};
			if (!parser(item, def, outError))
				return false;
			outDefs.push_back(std::move(def));
		}

		return true;
	}

	bool ParseBuffDocument(
		const json& root,
		BuffDef& outDef,
		std::string& outError)
	{
		if (!root.contains("profile") ||
			!ParseProfile(root.at("profile"), outDef.profile, outError))
		{
			return false;
		}

		if (!root.contains("lifetime") ||
			!ParseLifetime(root.at("lifetime"), outDef.lifetime, outError))
		{
			return false;
		}

		return
			ParseArray<BuffEffectDef>(root, "effects", outDef.effects, ParseEffect, outError) &&
			ParseArray<BuffApplyRequirementDef>(
				root,
				"applyRequirements",
				outDef.applyRequirements,
				ParseApplyRequirement,
				outError) &&
			ParseArray<BuffRemoveRuleDef>(
				root,
				"removeRules",
				outDef.removeRules,
				ParseRemoveRule,
				outError);
	}

	bool ValidateBuffDefs(std::span<const BuffDef> defs, std::string& outError)
	{
		for (const BuffDef& def : defs)
		{
			if (def.profile.id == BuffId::None)
			{
				outError = "Buff id must not be None.";
				return false;
			}

			if (def.profile.name.empty())
			{
				outError = "Buff name must not be empty.";
				return false;
			}
		}

		return true;
	}
}

const BuffDef* FindBuffDef(BuffId id) noexcept
{
	return GetBuffDefRegistry().Find(id);
}

const BuffDef& GetBuffDef(BuffId id)
{
	return GetBuffDefRegistry().Get(id);
}

std::span<const BuffDef> GetBuffDefs() noexcept
{
	return GetBuffDefRegistry().GetAll();
}

BuffDefLoadResult LoadBuffDefsFromJsonDirectory(
	const std::filesystem::path& directory)
{
	std::vector<DefJsonDocument> documents;
	BuffDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "Buff");
	if (!result.succeeded)
		return result;

	std::vector<BuffDef> defs(documents.size());
	for (size_t i = 0; i < documents.size(); ++i)
	{
		try
		{
			if (!ParseBuffDocument(documents[i].root, defs[i], result.error))
			{
				result.error = documents[i].path.string() + ": " + result.error;
				result.succeeded = false;
				return result;
			}
		}
		catch (const std::exception& ex)
		{
			result.error = documents[i].path.string() +
				": Invalid buff json field: " + std::string(ex.what());
			result.succeeded = false;
			return result;
		}
	}

	if (!ValidateBuffDefs(defs, result.error))
	{
		result.succeeded = false;
		return result;
	}

	BuffDefRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}

	BuffDefRegistry& activeRegistry = GetBuffDefRegistry();
	activeRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = activeRegistry.Size();
	result.error.clear();
	return result;
}
