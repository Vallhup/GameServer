#include "pch.h"
#include "SpawnSetDef.h"

#include "DefEnumString.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"

#include <vector>

using json = nlohmann::json;

namespace
{
	struct SpawnSetDefTraits
	{
		static SpawnSetId GetId(const SpawnSetDef& def) noexcept
		{
			return def.id;
		}
	};

	using SpawnSetDefRegistry =
		DefRegistry<SpawnSetDef, SpawnSetId, SpawnSetDefTraits>;

	SpawnSetDefRegistry& GetSpawnSetDefRegistry() noexcept
	{
		static SpawnSetDefRegistry registry;
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
		SpawnEntryDef& outEntry,
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
		SpawnSetDef& outDef,
		std::string& outError)
	{
		if (!ReadEnum(root, "id", outDef.id, "SpawnSetId", outError) ||
			!ReadRequiredString(root, "name", outDef.name, outError))
		{
			return false;
		}

		if (!root.contains("entries") || !root.at("entries").is_array())
		{
			outError = "SpawnSet entries must be an array.";
			return false;
		}

		const json& entries = root.at("entries");
		outDef.entries.clear();
		outDef.entries.reserve(entries.size());
		for (const json& entryNode : entries)
		{
			SpawnEntryDef entry{};
			if (!ParseEntry(entryNode, entry, outError))
				return false;

			outDef.entries.push_back(entry);
		}

		return true;
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
}

const SpawnSetDef* FindSpawnSetDef(SpawnSetId id) noexcept
{
	return GetSpawnSetDefRegistry().Find(id);
}

const SpawnSetDef& GetSpawnSetDef(SpawnSetId id)
{
	return GetSpawnSetDefRegistry().Get(id);
}

std::span<const SpawnSetDef> GetSpawnSetDefs() noexcept
{
	return GetSpawnSetDefRegistry().GetAll();
}

SpawnSetDefLoadResult LoadSpawnSetDefsFromJsonDirectory(
	const std::filesystem::path& directory)
{
	std::vector<DefJsonDocument> documents;
	SpawnSetDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "SpawnSet");
	if (!result.succeeded)
		return result;

	std::vector<SpawnSetDef> defs(documents.size());
	for (size_t i = 0; i < documents.size(); ++i)
	{
		try
		{
			if (!ParseSpawnSetDocument(documents[i].root, defs[i], result.error))
			{
				result.error = documents[i].path.string() + ": " + result.error;
				result.succeeded = false;
				return result;
			}
		}
		catch (const std::exception& ex)
		{
			result.error = documents[i].path.string() +
				": Invalid spawn set json field: " + std::string(ex.what());
			result.succeeded = false;
			return result;
		}
	}

	if (!ValidateSpawnSetDefs(defs, result.error))
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

	SpawnSetDefRegistry& activeRegistry = GetSpawnSetDefRegistry();
	activeRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = activeRegistry.Size();
	result.error.clear();
	return result;
}
