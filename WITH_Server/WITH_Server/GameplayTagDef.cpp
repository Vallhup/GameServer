#include "pch.h"
#include "GameplayTagDef.h"

#include "DefCompilePipeline.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"

#include <algorithm>

using json = nlohmann::json;

namespace
{
	struct GameplayTagDefDto
	{
		std::string key;
		std::string name;
		std::string path;
		std::vector<std::string> parentKeys;
	};

	bool ParseParentList(
		const json& node,
		std::vector<std::string>& outParentKeys,
		std::string& outError)
	{
		outParentKeys.clear();

		if (!node.contains("parents"))
			return true;

		if (!node.at("parents").is_array())
		{
			outError = "Gameplay tag parents must be an array.";
			return false;
		}

		const json& parentArray = node.at("parents");
		outParentKeys.reserve(parentArray.size());
		for (const json& parentNode : parentArray)
		{
			if (!parentNode.is_string())
			{
				outError = "Gameplay tag parent entry must be a string.";
				return false;
			}

			outParentKeys.push_back(parentNode.get<std::string>());
		}

		return true;
	}

	bool ParseGameplayTag(
		const json& node,
		GameplayTagDefDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ReadRequiredString(node, "name", outDto.name, outError) &&
			ReadRequiredString(node, "path", outDto.path, outError) &&
			ParseParentList(node, outDto.parentKeys, outError);
	}

	bool AppendGameplayTagDocumentDtos(
		const json& root,
		std::vector<GameplayTagDefDto>& outDtos,
		std::string& outError)
	{
		if (!root.contains("tags") || !root.at("tags").is_array())
		{
			outError = "Missing or invalid array field: tags";
			return false;
		}

		for (const json& tagNode : root.at("tags"))
		{
			GameplayTagDefDto dto{};
			if (!ParseGameplayTag(tagNode, dto, outError))
				return false;

			outDtos.push_back(std::move(dto));
		}

		return true;
	}

	bool BuildGameplayTagKeyIndex(
		std::span<const DefParsedDto<GameplayTagDefDto>> dtos,
		DefKeyIndex<GameplayTagId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const DefParsedDto<GameplayTagDefDto>& parsedDto : dtos)
			keys.push_back(parsedDto.dto.key);

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			static_cast<GameplayTagId>(InvalidGameplayTagId + 1),
			outIndex,
			outError);
	}

	bool CompileGameplayTagDef(
		const GameplayTagDefDto& dto,
		const DefKeyIndex<GameplayTagId>& keyIndex,
		GameplayTagDef& outDef,
		std::string& outError)
	{
		GameplayTagId id{ InvalidGameplayTagId };
		if (!keyIndex.FindId(dto.key, id))
		{
			outError = "Unknown gameplay tag key: " + dto.key;
			return false;
		}

		outDef.id = id;
		outDef.key = dto.key;
		outDef.name = dto.name;
		outDef.path = dto.path;
		outDef.parents.clear();
		outDef.parents.reserve(dto.parentKeys.size());

		for (const std::string& parentKey : dto.parentKeys)
		{
			GameplayTagId parentId{ InvalidGameplayTagId };
			if (!keyIndex.FindId(parentKey, parentId))
			{
				outError = "Unknown gameplay tag parent key: " + parentKey;
				return false;
			}

			outDef.parents.push_back(parentId);
		}

		return true;
	}
}

DefLoadResult LoadGameplayTagDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	GameplayTagDefRegistry& outRegistry)
{
	DefLoadResult result{};
	std::vector<DefJsonDocument> documents;
	result = LoadDefJsonDocumentsFromDirectory(
		directory,
		documents,
		"GameplayTag");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<GameplayTagDefDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendGameplayTagDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<GameplayTagId> keyIndex;
	if (!BuildGameplayTagKeyIndex(
			std::span<const DefParsedDto<GameplayTagDefDto>>(dtos.data(), dtos.size()),
			keyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<GameplayTagDef> defs;
	if (!CompileRuntimeDefs<GameplayTagDefDto, GameplayTagDef>(
			std::span<const DefParsedDto<GameplayTagDefDto>>(dtos.data(), dtos.size()),
			[&keyIndex](
				const GameplayTagDefDto& dto,
				GameplayTagDef& outDef,
				std::string& outError)
			{
				return CompileGameplayTagDef(dto, keyIndex, outDef, outError);
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
		[](const GameplayTagDef& lhs, const GameplayTagDef& rhs)
		{
			return lhs.id < rhs.id;
		});

	if (!ValidateGameplayTagDefs(
			std::span<const GameplayTagDef>(defs.data(), defs.size()),
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	GameplayTagDefRegistry registry;
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
