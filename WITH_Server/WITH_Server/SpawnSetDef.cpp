#include "pch.h"
#include "SpawnSetDef.h"

#include "DefCompilePipeline.h"
#include "DefEnumString.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"
#include "DefRegistry.h"

#include <algorithm>
#include <vector>

using json = nlohmann::json;

namespace
{
	struct SpawnEntryDefDto
	{
		CharacterId characterId{ CharacterId::None };
		SpawnPointId spawnPointId{ SpawnPointIds::None };
		uint8_t count{ 0 };
		SpawnConditionType type{ SpawnConditionType::Always };
		RespawnPolicy respawnPolicy{ RespawnPolicy::None };
		std::optional<float> respawnDelaySec;
	};

	struct SpawnSetDefDto
	{
		std::string key;
		std::string name;
		std::vector<SpawnEntryDefDto> entries;
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

	bool ReadSpawnPointId(
		const json& node,
		const char* field,
		SpawnPointId& outValue,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!ParseSpawnPointIdString(text, outValue))
		{
			outError = "Unknown SpawnPointId: " + text;
			return false;
		}

		return true;
	}

	bool ReadNullableRespawnDelay(
		const json& node,
		std::optional<float>& outValue,
		std::string& outError)
	{
		if (!node.contains("respawnDelaySec") ||
			node.at("respawnDelaySec").is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at("respawnDelaySec").is_number())
		{
			outError = "respawnDelaySec must be a number or null.";
			return false;
		}

		outValue = node.at("respawnDelaySec").get<float>();
		return true;
	}

	bool ParseEntry(
		const json& node,
		SpawnEntryDefDto& outEntry,
		std::string& outError)
	{
		return
			ReadEnum(node, "characterId", outEntry.characterId, "CharacterId", outError) &&
			ReadSpawnPointId(node, "spawnPointId", outEntry.spawnPointId, outError) &&
			ReadRequiredNumber(node, "count", outEntry.count, outError) &&
			ReadEnum(node, "type", outEntry.type, "SpawnConditionType", outError) &&
			ReadEnum(node, "respawnPolicy", outEntry.respawnPolicy, "RespawnPolicy", outError) &&
			ReadNullableRespawnDelay(node, outEntry.respawnDelaySec, outError);
	}

	bool ParseSpawnSetDocument(
		const json& root,
		SpawnSetDefDto& outDto,
		std::string& outError)
	{
		if (!ReadRequiredString(root, "key", outDto.key, outError) ||
			!ReadRequiredString(root, "name", outDto.name, outError))
		{
			return false;
		}

		if (!root.contains("entries") || !root.at("entries").is_array())
		{
			outError = "SpawnSet entries must be an array.";
			return false;
		}

		const json& entries = root.at("entries");
		outDto.entries.clear();
		outDto.entries.reserve(entries.size());
		for (const json& entryNode : entries)
		{
			SpawnEntryDefDto entry{};
			if (!ParseEntry(entryNode, entry, outError))
				return false;

			outDto.entries.push_back(entry);
		}

		return true;
	}

	bool BuildSpawnSetKeyIndex(
		std::span<const DefParsedDto<SpawnSetDefDto>> dtos,
		DefKeyIndex<SpawnSetId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const DefParsedDto<SpawnSetDefDto>& parsedDto : dtos)
			keys.push_back(parsedDto.dto.key);

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			static_cast<SpawnSetId>(1),
			outIndex,
			outError);
	}

	bool ValidateSpawnSetDefs(
		std::span<const SpawnSetDef> defs,
		std::string& outError)
	{
		for (const SpawnSetDef& def : defs)
		{
			if (def.id == SpawnSetId::None)
			{
				outError = "SpawnSet id must not be None.";
				return false;
			}

			if (def.key.empty())
			{
				outError = "SpawnSet key must not be empty.";
				return false;
			}

			if (def.name.empty())
			{
				outError = "SpawnSet name must not be empty.";
				return false;
			}

			for (const SpawnEntryDef& entry : def.entries)
			{
				if (entry.characterId == CharacterId::None)
				{
					outError = "SpawnEntry characterId must not be None.";
					return false;
				}

				if (entry.spawnPointId == SpawnPointIds::None)
				{
					outError = "SpawnEntry spawnPointId must not be None.";
					return false;
				}

				if (entry.count == 0)
				{
					outError = "SpawnEntry count must not be zero.";
					return false;
				}
			}
		}

		return true;
	}

	bool AppendSpawnSetDocumentDtos(
		const json& root,
		std::vector<SpawnSetDefDto>& outDtos,
		std::string& outError)
	{
		SpawnSetDefDto dto{};
		if (!ParseSpawnSetDocument(root, dto, outError))
			return false;

		outDtos.push_back(std::move(dto));
		return true;
	}

	bool CompileSpawnSetDef(
		const SpawnSetDefDto& dto,
		const DefKeyIndex<SpawnSetId>& keyIndex,
		SpawnSetDef& outDef,
		std::string& outError)
	{
		SpawnSetId id{ SpawnSetId::None };
		if (!keyIndex.FindId(dto.key, id))
		{
			outError = "Unknown SpawnSet key: " + dto.key;
			return false;
		}

		outDef.id = id;
		outDef.key = dto.key;
		outDef.name = dto.name;
		outDef.entries.clear();
		outDef.entries.reserve(dto.entries.size());

		for (const SpawnEntryDefDto& entryDto : dto.entries)
		{
			outDef.entries.push_back(SpawnEntryDef{
				.characterId = entryDto.characterId,
				.spawnPointId = entryDto.spawnPointId,
				.count = entryDto.count,
				.type = entryDto.type,
				.respawnPolicy = entryDto.respawnPolicy,
				.respawnDelaySec = entryDto.respawnDelaySec
			});
		}

		return true;
	}
}

SpawnSetDefLoadResult LoadSpawnSetDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	SpawnSetDefRegistry& outRegistry)
{
	DefLoadResult result{};
	std::vector<DefJsonDocument> documents;
	result = LoadDefJsonDocumentsFromDirectory(
		directory,
		documents,
		"SpawnSet");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<SpawnSetDefDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendSpawnSetDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<SpawnSetId> keyIndex;
	if (!BuildSpawnSetKeyIndex(
			std::span<const DefParsedDto<SpawnSetDefDto>>(dtos.data(), dtos.size()),
			keyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<SpawnSetDef> defs;
	if (!CompileRuntimeDefs<SpawnSetDefDto, SpawnSetDef>(
			std::span<const DefParsedDto<SpawnSetDefDto>>(dtos.data(), dtos.size()),
			[&keyIndex](
				const SpawnSetDefDto& dto,
				SpawnSetDef& outDef,
				std::string& outError)
			{
				return CompileSpawnSetDef(dto, keyIndex, outDef, outError);
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
		[](const SpawnSetDef& lhs, const SpawnSetDef& rhs)
		{
			return lhs.id < rhs.id;
		});

	if (!ValidateSpawnSetDefs(
			std::span<const SpawnSetDef>(defs.data(), defs.size()),
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	SpawnSetDefRegistry registry;
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
