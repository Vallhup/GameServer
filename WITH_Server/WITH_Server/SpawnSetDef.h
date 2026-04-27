#pragma once

#include "EntityId.h"
#include "WorldContentIds.h"
#include "IDs.h"
#include "DefLoadResult.h"
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
	SpawnSetId id;
	std::string name;

	std::vector<SpawnEntryDef> entries;
};

const SpawnSetDef* FindSpawnSetDef(SpawnSetId id) noexcept;
const SpawnSetDef& GetSpawnSetDef(SpawnSetId id);
std::span<const SpawnSetDef> GetSpawnSetDefs() noexcept;

using SpawnSetDefLoadResult = DefLoadResult;
SpawnSetDefLoadResult LoadSpawnSetDefsFromJsonDirectory(
	const std::filesystem::path& directory);
