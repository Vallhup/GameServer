#pragma once

#include "EntityId.h"
#include "WorldContentIds.h"
#include "DefLoadResult.h"
#include "DefRegistry.h"
#include <filesystem>
#include <span>
#include <string>
#include <optional>
#include <vector>

enum class SpawnConditionType : uint8_t
{
	Always,
	OnWorldStart,
	OnRespawn
};

enum class RespawnPolicy : uint8_t
{
	None,
	Delay
};

struct SpawnEntryDef
{
	CharacterId characterId;
	SpawnPointId spawnPointId;
	uint8_t count;

	SpawnConditionType type;

	RespawnPolicy respawnPolicy;
	std::optional<float> respawnDelaySec;
};

struct SpawnSetDef
{
	SpawnSetId id{ SpawnSetId::None };
	std::string key;
	std::string name;

	std::vector<SpawnEntryDef> entries;
};

struct SpawnSetDefTraits
{
	static SpawnSetId GetId(const SpawnSetDef& def) noexcept
	{
		return def.id;
	}
};

using SpawnSetDefRegistry =
	DefRegistry<SpawnSetDef, SpawnSetId, SpawnSetDefTraits>;

using SpawnSetDefLoadResult = DefLoadResult;
SpawnSetDefLoadResult LoadSpawnSetDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	SpawnSetDefRegistry& outRegistry);
