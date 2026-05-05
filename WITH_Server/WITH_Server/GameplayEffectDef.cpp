#include "pch.h"
#include "GameplayEffectDef.h"

#include "DefCompilePipeline.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"

#include <algorithm>

using json = nlohmann::json;

namespace
{
	struct GameplayTagQueryDto
	{
		GameplayTagQueryOp op{ GameplayTagQueryOp::None };
		std::vector<std::string> tagKeys;
		std::vector<GameplayTagQueryDto> children;
	};

	struct AttributeModifierDto
	{
		std::string attributeKey;
		AttributeModifierOp op{ AttributeModifierOp::Add };
		float value{ 0.0f };
	};

	struct GameplayTagModifierDto
	{
		std::string tagKey;
		GameplayTagModifierOp op{ GameplayTagModifierOp::Add };
	};

	struct PeriodicEffectDto
	{
		PeriodicEffectKind kind{ PeriodicEffectKind::None };
		std::string attributeKey;
		float value{ 0.0f };
	};

	struct GameplayEffectRequirementDto
	{
		GameplayTagQueryDto requiredTags;
		GameplayTagQueryDto blockedTags;
	};

	struct GameplayEffectRemoveRuleDto
	{
		GameplayEffectRemoveRuleTrigger trigger{
			GameplayEffectRemoveRuleTrigger::TagQueryMatched };
		GameplayTagQueryDto removeWhenTagsMatch;
		std::optional<float> counterThreshold;
		std::optional<std::string> abilityKind;
		GameplayEffectCounterEvent counterEvent{
			GameplayEffectCounterEvent::None };
	};

	struct GameplayEffectDefDto
	{
		std::string key;
		std::string name;
		GameplayEffectKind kind{ GameplayEffectKind::None };
		std::vector<std::string> tagKeys;
		GameplayEffectLifetimeDef lifetime;
		GameplayEffectStackingDef stacking;
		std::vector<AttributeModifierDto> attributeModifiers;
		std::vector<GameplayTagModifierDto> tagModifiers;
		std::vector<PeriodicEffectDto> periodicEffects;
		std::vector<GameplayEffectRequirementDto> applyRequirements;
		std::vector<GameplayEffectRemoveRuleDto> removeRules;
	};

	template<typename TEnum, typename TParser>
	bool ReadTypedString(
		const json& node,
		const char* field,
		TEnum& outValue,
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

	bool ParseGameplayEffectKind(
		std::string_view text,
		GameplayEffectKind& outValue) noexcept
	{
		if (text == "Positive") { outValue = GameplayEffectKind::Positive; return true; }
		if (text == "Negative") { outValue = GameplayEffectKind::Negative; return true; }
		if (text == "Control") { outValue = GameplayEffectKind::Control; return true; }
		if (text == "None") { outValue = GameplayEffectKind::None; return true; }
		return false;
	}

	bool ParseDurationPolicy(
		std::string_view text,
		GameplayEffectDurationPolicy& outValue) noexcept
	{
		if (text == "Instant") { outValue = GameplayEffectDurationPolicy::Instant; return true; }
		if (text == "Timed") { outValue = GameplayEffectDurationPolicy::Timed; return true; }
		if (text == "Infinite") { outValue = GameplayEffectDurationPolicy::Infinite; return true; }
		return false;
	}

	bool ParseStackPolicy(
		std::string_view text,
		GameplayEffectStackPolicy& outValue) noexcept
	{
		if (text == "Independent") { outValue = GameplayEffectStackPolicy::Independent; return true; }
		if (text == "Refresh") { outValue = GameplayEffectStackPolicy::Refresh; return true; }
		if (text == "Stack") { outValue = GameplayEffectStackPolicy::Stack; return true; }
		if (text == "Replace") { outValue = GameplayEffectStackPolicy::Replace; return true; }
		return false;
	}

	bool ParseAttributeModifierOp(
		std::string_view text,
		AttributeModifierOp& outValue) noexcept
	{
		if (text == "Add") { outValue = AttributeModifierOp::Add; return true; }
		if (text == "Multiply") { outValue = AttributeModifierOp::Multiply; return true; }
		return false;
	}

	bool ParseTagModifierOp(
		std::string_view text,
		GameplayTagModifierOp& outValue) noexcept
	{
		if (text == "Add") { outValue = GameplayTagModifierOp::Add; return true; }
		if (text == "Remove") { outValue = GameplayTagModifierOp::Remove; return true; }
		return false;
	}

	bool ParsePeriodicEffectKind(
		std::string_view text,
		PeriodicEffectKind& outValue) noexcept
	{
		if (text == "None") { outValue = PeriodicEffectKind::None; return true; }
		if (text == "AttributeDelta") { outValue = PeriodicEffectKind::AttributeDelta; return true; }
		return false;
	}

	bool ParseRemoveRuleTrigger(
		std::string_view text,
		GameplayEffectRemoveRuleTrigger& outValue) noexcept
	{
		if (text == "TagQueryMatched") { outValue = GameplayEffectRemoveRuleTrigger::TagQueryMatched; return true; }
		if (text == "CounterReached") { outValue = GameplayEffectRemoveRuleTrigger::CounterReached; return true; }
		if (text == "DurationExpired") { outValue = GameplayEffectRemoveRuleTrigger::DurationExpired; return true; }
		return false;
	}

	bool ParseCounterEvent(
		std::string_view text,
		GameplayEffectCounterEvent& outValue) noexcept
	{
		if (text == "None") { outValue = GameplayEffectCounterEvent::None; return true; }
		if (text == "AbilityCommitted") { outValue = GameplayEffectCounterEvent::AbilityCommitted; return true; }
		return false;
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

	bool ParseTagKeyArray(
		const json& node,
		std::vector<std::string>& outTagKeys,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Gameplay tag list must be an array.";
			return false;
		}

		outTagKeys.clear();
		outTagKeys.reserve(node.size());
		for (const json& tagNode : node)
		{
			if (!tagNode.is_string())
			{
				outError = "Gameplay tag entry must be a string.";
				return false;
			}

			outTagKeys.push_back(tagNode.get<std::string>());
		}

		return true;
	}

	bool AddTagToMask(
		GameplayTagId tagId,
		GameplayTagMask& outMask,
		std::string& outError)
	{
		if (tagId == InvalidGameplayTagId || tagId > 64)
		{
			outError = "Gameplay tag id cannot be represented in GameplayTagMask.";
			return false;
		}

		outMask |= (GameplayTagMask{ 1 } << (tagId - 1));
		return true;
	}

	bool ParseTagQuery(
		const json& node,
		GameplayTagQueryDto& outQuery,
		std::string& outError)
	{
		if (node.is_null())
		{
			outQuery = {};
			return true;
		}

		if (!node.is_object())
		{
			outError = "Gameplay tag query must be an object or null.";
			return false;
		}

		std::string opText;
		if (!ReadRequiredString(node, "op", opText, outError))
			return false;

		if (opText == "None") { outQuery.op = GameplayTagQueryOp::None; }
		else if (opText == "All") { outQuery.op = GameplayTagQueryOp::All; }
		else if (opText == "Any") { outQuery.op = GameplayTagQueryOp::Any; }
		else if (opText == "Not") { outQuery.op = GameplayTagQueryOp::Not; }
		else
		{
			outError = "Unknown GameplayTagQueryOp: " + opText;
			return false;
		}

		if (node.contains("tags") &&
			!ParseTagKeyArray(node.at("tags"), outQuery.tagKeys, outError))
		{
			return false;
		}

		if (node.contains("children"))
		{
			if (!node.at("children").is_array())
			{
				outError = "Gameplay tag query children must be an array.";
				return false;
			}

			outQuery.children.clear();
			for (const json& childNode : node.at("children"))
			{
				GameplayTagQueryDto child{};
				if (!ParseTagQuery(childNode, child, outError))
					return false;

				outQuery.children.push_back(std::move(child));
			}
		}

		return true;
	}

	template<typename TDef, typename TParser>
	bool ParseArray(
		const json& node,
		const char* field,
		std::vector<TDef>& outDefs,
		TParser parser,
		std::string& outError)
	{
		outDefs.clear();
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_array())
		{
			outError = std::string("Invalid array field: ") + field;
			return false;
		}

		const json& arrayNode = node.at(field);
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

	bool ParseLifetime(
		const json& node,
		GameplayEffectLifetimeDef& outLifetime,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"durationPolicy",
				outLifetime.durationPolicy,
				"GameplayEffectDurationPolicy",
				ParseDurationPolicy,
				outError) &&
			ReadRequiredNumber(
				node,
				"defaultDurationSec",
				outLifetime.defaultDurationSec,
				outError) &&
			ReadNullableNumber(
				node,
				"tickIntervalSec",
				outLifetime.tickIntervalSec,
				outError);
	}

	bool ParseStacking(
		const json& node,
		GameplayEffectStackingDef& outStacking,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "groupKey", outStacking.groupKey, outError) &&
			ReadRequiredNumber(
				node,
				"maxStackCount",
				outStacking.maxStackCount,
				outError) &&
			ReadTypedString(
				node,
				"stackPolicy",
				outStacking.stackPolicy,
				"GameplayEffectStackPolicy",
				ParseStackPolicy,
				outError);
	}

	bool ParseAttributeModifier(
		const json& node,
		AttributeModifierDto& outModifier,
		std::string& outError)
	{
		return
			ReadRequiredString(
				node,
				"attributeKey",
				outModifier.attributeKey,
				outError) &&
			ReadTypedString(
				node,
				"op",
				outModifier.op,
				"AttributeModifierOp",
				ParseAttributeModifierOp,
				outError) &&
			ReadRequiredNumber(node, "value", outModifier.value, outError);
	}

	bool ParseTagModifier(
		const json& node,
		GameplayTagModifierDto& outModifier,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "tagKey", outModifier.tagKey, outError) &&
			ReadTypedString(
				node,
				"op",
				outModifier.op,
				"GameplayTagModifierOp",
				ParseTagModifierOp,
				outError);
	}

	bool ParsePeriodicEffect(
		const json& node,
		PeriodicEffectDto& outEffect,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"kind",
				outEffect.kind,
				"PeriodicEffectKind",
				ParsePeriodicEffectKind,
				outError) &&
			ReadRequiredString(
				node,
				"attributeKey",
				outEffect.attributeKey,
				outError) &&
			ReadRequiredNumber(node, "value", outEffect.value, outError);
	}

	bool ParseRequirement(
		const json& node,
		GameplayEffectRequirementDto& outRequirement,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Gameplay effect requirement must be an object.";
			return false;
		}

		if (node.contains("requiredTags") &&
			!ParseTagQuery(
				node.at("requiredTags"),
				outRequirement.requiredTags,
				outError))
		{
			return false;
		}

		if (node.contains("blockedTags") &&
			!ParseTagQuery(
				node.at("blockedTags"),
				outRequirement.blockedTags,
				outError))
		{
			return false;
		}

		return true;
	}

	bool ParseRemoveRule(
		const json& node,
		GameplayEffectRemoveRuleDto& outRule,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Gameplay effect remove rule must be an object.";
			return false;
		}

		if (node.contains("trigger"))
		{
			if (!ReadTypedString(
					node,
					"trigger",
					outRule.trigger,
					"GameplayEffectRemoveRuleTrigger",
					ParseRemoveRuleTrigger,
					outError))
			{
				return false;
			}
		}

		if (node.contains("removeWhenTagsMatch") &&
			!ParseTagQuery(
				node.at("removeWhenTagsMatch"),
				outRule.removeWhenTagsMatch,
				outError))
		{
			return false;
		}

		if (node.contains("counterThreshold") &&
			!node.at("counterThreshold").is_null())
		{
			if (!node.at("counterThreshold").is_number())
			{
				outError = "Invalid numeric field: counterThreshold";
				return false;
			}

			outRule.counterThreshold =
				node.at("counterThreshold").get<float>();
		}

		if (node.contains("abilityKind") && !node.at("abilityKind").is_null())
		{
			if (!node.at("abilityKind").is_string())
			{
				outError = "Invalid string field: abilityKind";
				return false;
			}

			outRule.abilityKind = node.at("abilityKind").get<std::string>();
		}

		if (node.contains("counterEvent"))
		{
			if (!ReadTypedString(
					node,
					"counterEvent",
					outRule.counterEvent,
					"GameplayEffectCounterEvent",
					ParseCounterEvent,
					outError))
			{
				return false;
			}
		}

		return true;
	}

	bool ParseGameplayEffectDocument(
		const json& root,
		GameplayEffectDefDto& outDto,
		std::string& outError)
	{
		if (!ReadRequiredString(root, "key", outDto.key, outError) ||
			!ReadRequiredString(root, "name", outDto.name, outError) ||
			!ReadTypedString(
				root,
				"kind",
				outDto.kind,
				"GameplayEffectKind",
				ParseGameplayEffectKind,
				outError))
		{
			return false;
		}

		if (root.contains("tags") &&
			!ParseTagKeyArray(root.at("tags"), outDto.tagKeys, outError))
		{
			return false;
		}

		if (!root.contains("lifetime") ||
			!ParseLifetime(root.at("lifetime"), outDto.lifetime, outError))
		{
			return false;
		}

		if (!root.contains("stacking") ||
			!ParseStacking(root.at("stacking"), outDto.stacking, outError))
		{
			return false;
		}

		return
			ParseArray<AttributeModifierDto>(
				root,
				"attributeModifiers",
				outDto.attributeModifiers,
				ParseAttributeModifier,
				outError) &&
			ParseArray<GameplayTagModifierDto>(
				root,
				"tagModifiers",
				outDto.tagModifiers,
				ParseTagModifier,
				outError) &&
			ParseArray<PeriodicEffectDto>(
				root,
				"periodicEffects",
				outDto.periodicEffects,
				ParsePeriodicEffect,
				outError) &&
			ParseArray<GameplayEffectRequirementDto>(
				root,
				"applyRequirements",
				outDto.applyRequirements,
				ParseRequirement,
				outError) &&
			ParseArray<GameplayEffectRemoveRuleDto>(
				root,
				"removeRules",
				outDto.removeRules,
				ParseRemoveRule,
				outError);
	}

	bool AppendGameplayEffectDocumentDtos(
		const json& root,
		std::vector<GameplayEffectDefDto>& outDtos,
		std::string& outError)
	{
		if (root.contains("effects"))
		{
			if (!root.at("effects").is_array())
			{
				outError = "Missing or invalid array field: effects";
				return false;
			}

			for (const json& effectNode : root.at("effects"))
			{
				GameplayEffectDefDto dto{};
				if (!ParseGameplayEffectDocument(effectNode, dto, outError))
					return false;

				outDtos.push_back(std::move(dto));
			}

			return true;
		}

		GameplayEffectDefDto dto{};
		if (!ParseGameplayEffectDocument(root, dto, outError))
			return false;

		outDtos.push_back(std::move(dto));
		return true;
	}

	template<typename TDef, typename TId>
	bool ResolveDefIdByKey(
		std::span<const TDef> defs,
		std::string_view key,
		TId& outId,
		const char* typeName,
		std::string& outError)
	{
		for (const TDef& def : defs)
		{
			if (def.key == key)
			{
				outId = def.id;
				return true;
			}
		}

		outError = std::string("Unknown ") + typeName + " key: " +
			std::string(key);
		return false;
	}

	bool CompileTagQuery(
		const GameplayTagQueryDto& dto,
		std::span<const GameplayTagDef> tags,
		GameplayTagQueryDef& outQuery,
		std::string& outError)
	{
		outQuery.op = dto.op;
		outQuery.tags.clear();
		outQuery.tags.reserve(dto.tagKeys.size());
		for (const std::string& tagKey : dto.tagKeys)
		{
			GameplayTagId tagId{ InvalidGameplayTagId };
			if (!ResolveDefIdByKey(
					tags,
					tagKey,
					tagId,
					"gameplay tag",
					outError))
			{
				return false;
			}

			outQuery.tags.push_back(tagId);
		}

		outQuery.children.clear();
		outQuery.children.reserve(dto.children.size());
		for (const GameplayTagQueryDto& childDto : dto.children)
		{
			GameplayTagQueryDef child{};
			if (!CompileTagQuery(childDto, tags, child, outError))
				return false;

			outQuery.children.push_back(std::move(child));
		}

		return true;
	}

	bool CompileTagMask(
		std::span<const std::string> tagKeys,
		std::span<const GameplayTagDef> tags,
		GameplayTagMask& outMask,
		std::string& outError)
	{
		outMask = 0;
		for (const std::string& tagKey : tagKeys)
		{
			GameplayTagId tagId{ InvalidGameplayTagId };
			if (!ResolveDefIdByKey(
					tags,
					tagKey,
					tagId,
					"gameplay tag",
					outError) ||
				!AddTagToMask(tagId, outMask, outError))
			{
				return false;
			}
		}

		return true;
	}

	bool CompileAttributeModifier(
		const AttributeModifierDto& dto,
		std::span<const AttributeDef> attributes,
		AttributeModifierDef& outModifier,
		std::string& outError)
	{
		if (!ResolveDefIdByKey(
				attributes,
				dto.attributeKey,
				outModifier.attributeId,
				"attribute",
				outError))
		{
			return false;
		}

		outModifier.op = dto.op;
		outModifier.value = dto.value;
		return true;
	}

	bool CompileTagModifier(
		const GameplayTagModifierDto& dto,
		std::span<const GameplayTagDef> tags,
		GameplayTagModifierDef& outModifier,
		std::string& outError)
	{
		if (!ResolveDefIdByKey(
				tags,
				dto.tagKey,
				outModifier.tagId,
				"gameplay tag",
				outError))
		{
			return false;
		}

		outModifier.op = dto.op;
		return true;
	}

	bool CompilePeriodicEffect(
		const PeriodicEffectDto& dto,
		std::span<const AttributeDef> attributes,
		PeriodicEffectDef& outEffect,
		std::string& outError)
	{
		if (!ResolveDefIdByKey(
				attributes,
				dto.attributeKey,
				outEffect.attributeId,
				"attribute",
				outError))
		{
			return false;
		}

		outEffect.kind = dto.kind;
		outEffect.value = dto.value;
		return true;
	}

	bool CompileRequirement(
		const GameplayEffectRequirementDto& dto,
		std::span<const GameplayTagDef> tags,
		GameplayEffectRequirementDef& outRequirement,
		std::string& outError)
	{
		return
			CompileTagQuery(
				dto.requiredTags,
				tags,
				outRequirement.requiredTags,
				outError) &&
			CompileTagQuery(
				dto.blockedTags,
				tags,
				outRequirement.blockedTags,
				outError);
	}

	bool CompileRemoveRule(
		const GameplayEffectRemoveRuleDto& dto,
		std::span<const GameplayTagDef> tags,
		GameplayEffectRemoveRuleDef& outRule,
		std::string& outError)
	{
		outRule.trigger = dto.trigger;
		outRule.counterThreshold = dto.counterThreshold;
		outRule.abilityKind = dto.abilityKind;
		outRule.counterEvent = dto.counterEvent;
		return CompileTagQuery(
			dto.removeWhenTagsMatch,
			tags,
			outRule.removeWhenTagsMatch,
			outError);
	}

	template<typename TDto, typename TDef, typename TCompiler>
	bool CompileVector(
		std::span<const TDto> dtos,
		std::vector<TDef>& outDefs,
		TCompiler compiler,
		std::string& outError)
	{
		outDefs.clear();
		outDefs.reserve(dtos.size());
		for (const TDto& dto : dtos)
		{
			TDef def{};
			if (!compiler(dto, def, outError))
				return false;

			outDefs.push_back(std::move(def));
		}

		return true;
	}

	bool BuildGameplayEffectKeyIndex(
		std::span<const DefParsedDto<GameplayEffectDefDto>> dtos,
		DefKeyIndex<GameplayEffectId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const DefParsedDto<GameplayEffectDefDto>& parsedDto : dtos)
			keys.push_back(parsedDto.dto.key);

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			static_cast<GameplayEffectId>(InvalidGameplayEffectId + 1),
			outIndex,
			outError);
	}

	bool CompileGameplayEffectDef(
		const GameplayEffectDefDto& dto,
		const DefKeyIndex<GameplayEffectId>& keyIndex,
		std::span<const AttributeDef> attributes,
		std::span<const GameplayTagDef> tags,
		GameplayEffectDef& outDef,
		std::string& outError)
	{
		GameplayEffectId id{ InvalidGameplayEffectId };
		if (!keyIndex.FindId(dto.key, id))
		{
			outError = "Unknown gameplay effect key: " + dto.key;
			return false;
		}

		outDef.id = id;
		outDef.key = dto.key;
		outDef.name = dto.name;
		outDef.kind = dto.kind;
		outDef.lifetime = dto.lifetime;
		outDef.stacking = dto.stacking;

		return
			CompileTagMask(
				std::span<const std::string>(
					dto.tagKeys.data(),
					dto.tagKeys.size()),
				tags,
				outDef.tags,
				outError) &&
			CompileVector<AttributeModifierDto, AttributeModifierDef>(
				std::span<const AttributeModifierDto>(
					dto.attributeModifiers.data(),
					dto.attributeModifiers.size()),
				outDef.attributeModifiers,
				[attributes](
					const AttributeModifierDto& modifierDto,
					AttributeModifierDef& modifierDef,
					std::string& error)
				{
					return CompileAttributeModifier(
						modifierDto,
						attributes,
						modifierDef,
						error);
				},
				outError) &&
			CompileVector<GameplayTagModifierDto, GameplayTagModifierDef>(
				std::span<const GameplayTagModifierDto>(
					dto.tagModifiers.data(),
					dto.tagModifiers.size()),
				outDef.tagModifiers,
				[tags](
					const GameplayTagModifierDto& modifierDto,
					GameplayTagModifierDef& modifierDef,
					std::string& error)
				{
					return CompileTagModifier(
						modifierDto,
						tags,
						modifierDef,
						error);
				},
				outError) &&
			CompileVector<PeriodicEffectDto, PeriodicEffectDef>(
				std::span<const PeriodicEffectDto>(
					dto.periodicEffects.data(),
					dto.periodicEffects.size()),
				outDef.periodicEffects,
				[attributes](
					const PeriodicEffectDto& effectDto,
					PeriodicEffectDef& effectDef,
					std::string& error)
				{
					return CompilePeriodicEffect(
						effectDto,
						attributes,
						effectDef,
						error);
				},
				outError) &&
			CompileVector<
				GameplayEffectRequirementDto,
				GameplayEffectRequirementDef>(
				std::span<const GameplayEffectRequirementDto>(
					dto.applyRequirements.data(),
					dto.applyRequirements.size()),
				outDef.applyRequirements,
				[tags](
					const GameplayEffectRequirementDto& requirementDto,
					GameplayEffectRequirementDef& requirementDef,
					std::string& error)
				{
					return CompileRequirement(
						requirementDto,
						tags,
						requirementDef,
						error);
				},
				outError) &&
			CompileVector<
				GameplayEffectRemoveRuleDto,
				GameplayEffectRemoveRuleDef>(
				std::span<const GameplayEffectRemoveRuleDto>(
					dto.removeRules.data(),
					dto.removeRules.size()),
				outDef.removeRules,
				[tags](
					const GameplayEffectRemoveRuleDto& ruleDto,
					GameplayEffectRemoveRuleDef& ruleDef,
					std::string& error)
				{
					return CompileRemoveRule(ruleDto, tags, ruleDef, error);
				},
				outError);
	}
}

DefLoadResult LoadGameplayEffectDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayTagDef> tags,
	GameplayEffectDefRegistry& outRegistry)
{
	DefLoadResult result{};
	std::vector<DefJsonDocument> documents;
	result = LoadDefJsonDocumentsFromDirectory(
		directory,
		documents,
		"GameplayEffect");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<GameplayEffectDefDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendGameplayEffectDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<GameplayEffectId> keyIndex;
	if (!BuildGameplayEffectKeyIndex(
			std::span<const DefParsedDto<GameplayEffectDefDto>>(
				dtos.data(),
				dtos.size()),
			keyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<GameplayEffectDef> defs;
	if (!CompileRuntimeDefs<GameplayEffectDefDto, GameplayEffectDef>(
			std::span<const DefParsedDto<GameplayEffectDefDto>>(
				dtos.data(),
				dtos.size()),
			[&keyIndex, attributes, tags](
				const GameplayEffectDefDto& dto,
				GameplayEffectDef& outDef,
				std::string& outError)
			{
				return CompileGameplayEffectDef(
					dto,
					keyIndex,
					attributes,
					tags,
					outDef,
					outError);
			},
			defs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::sort(
		defs.begin(),
		defs.end(),
		[](const GameplayEffectDef& lhs, const GameplayEffectDef& rhs)
		{
			return lhs.id < rhs.id;
		});

	if (!ValidateGameplayEffectDefs(
			std::span<const GameplayEffectDef>(defs.data(), defs.size()),
			attributes,
			tags,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	GameplayEffectDefRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}

	outRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = outRegistry.Size();
	result.error.clear();
	return result;
}
