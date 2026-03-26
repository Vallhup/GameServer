#pragma once

#include "EntityId.h"
#include "IDs.h"
#include <string>
#include <optional>

using SpawnPointId = uint16_t;

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
