#include "pch.h"
#include "AttributeDef.h"

#include "DefCompilePipeline.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"

#include <algorithm>

using json = nlohmann::json;

namespace
{
	struct AttributeDefDto
	{
		std::string key;
		std::string name;
		AttributeValueKind valueKind{ AttributeValueKind::Scalar };
		float defaultValue{ 0.0f };
		float minValue{ 0.0f };
		float maxValue{ 0.0f };
		bool clampToRange{ false };
	};

	bool ParseAttributeValueKind(
		std::string_view text,
		AttributeValueKind& outValue) noexcept
	{
		if (text == "Scalar") { outValue = AttributeValueKind::Scalar; return true; }
		if (text == "Resource") { outValue = AttributeValueKind::Resource; return true; }
		return false;
	}

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

	bool ParseAttribute(
		const json& node,
		AttributeDefDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ReadRequiredString(node, "name", outDto.name, outError) &&
			ReadTypedString(
				node,
				"valueKind",
				outDto.valueKind,
				"AttributeValueKind",
				ParseAttributeValueKind,
				outError) &&
			ReadRequiredNumber(
				node,
				"defaultValue",
				outDto.defaultValue,
				outError) &&
			ReadRequiredNumber(node, "minValue", outDto.minValue, outError) &&
			ReadRequiredNumber(node, "maxValue", outDto.maxValue, outError) &&
			ReadRequiredBool(
				node,
				"clampToRange",
				outDto.clampToRange,
				outError);
	}

	bool AppendAttributeDocumentDtos(
		const json& root,
		std::vector<AttributeDefDto>& outDtos,
		std::string& outError)
	{
		if (!root.contains("attributes") || !root.at("attributes").is_array())
		{
			outError = "Missing or invalid array field: attributes";
			return false;
		}

		for (const json& attributeNode : root.at("attributes"))
		{
			AttributeDefDto dto{};
			if (!ParseAttribute(attributeNode, dto, outError))
				return false;

			outDtos.push_back(std::move(dto));
		}

		return true;
	}

	bool BuildAttributeKeyIndex(
		std::span<const DefParsedDto<AttributeDefDto>> dtos,
		DefKeyIndex<AttributeId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const DefParsedDto<AttributeDefDto>& parsedDto : dtos)
			keys.push_back(parsedDto.dto.key);

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			static_cast<AttributeId>(InvalidAttributeId + 1),
			outIndex,
			outError);
	}

	bool CompileAttributeDef(
		const AttributeDefDto& dto,
		const DefKeyIndex<AttributeId>& keyIndex,
		AttributeDef& outDef,
		std::string& outError)
	{
		AttributeId id{ InvalidAttributeId };
		if (!keyIndex.FindId(dto.key, id))
		{
			outError = "Unknown attribute key: " + dto.key;
			return false;
		}

		outDef.id = id;
		outDef.key = dto.key;
		outDef.name = dto.name;
		outDef.valueKind = dto.valueKind;
		outDef.defaultValue = dto.defaultValue;
		outDef.minValue = dto.minValue;
		outDef.maxValue = dto.maxValue;
		outDef.clampToRange = dto.clampToRange;
		return true;
	}
}

DefLoadResult LoadAttributeDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AttributeDefRegistry& outRegistry)
{
	DefLoadResult result{};
	std::vector<DefJsonDocument> documents;
	result = LoadDefJsonDocumentsFromDirectory(
		directory,
		documents,
		"Attribute");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<AttributeDefDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendAttributeDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<AttributeId> keyIndex;
	if (!BuildAttributeKeyIndex(
			std::span<const DefParsedDto<AttributeDefDto>>(dtos.data(), dtos.size()),
			keyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<AttributeDef> defs;
	if (!CompileRuntimeDefs<AttributeDefDto, AttributeDef>(
			std::span<const DefParsedDto<AttributeDefDto>>(dtos.data(), dtos.size()),
			[&keyIndex](
				const AttributeDefDto& dto,
				AttributeDef& outDef,
				std::string& outError)
			{
				return CompileAttributeDef(dto, keyIndex, outDef, outError);
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
		[](const AttributeDef& lhs, const AttributeDef& rhs)
		{
			return lhs.id < rhs.id;
		});

	if (!ValidateAttributeDefs(
			std::span<const AttributeDef>(defs.data(), defs.size()),
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	AttributeDefRegistry registry;
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
