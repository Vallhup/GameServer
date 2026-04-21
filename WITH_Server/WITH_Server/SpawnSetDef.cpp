#include "pch.h"
#include "SpawnSetDef.h"

#include <array>

namespace
{
	const std::array<SpawnSetDef, 5> kSpawnSetDefs
	{
		SpawnSetDef{
			.id = SpawnSetId::PlazaDefault,
			.name = "PlazaDefault",
			.entries = {}
		},
		SpawnSetDef{
			.id = SpawnSetId::VillageDefault,
			.name = "VillageDefault",
			.entries = {
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::VillageMonster01,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::VillageMonster02,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::DemonStriker,
					.spawnPointId = SpawnPointIds::VillageMonster03,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::DemonExecutioner,
					.spawnPointId = SpawnPointIds::VillageMonster04,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::VillageMonster05,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
			},
		},
		SpawnSetDef{
			.id = SpawnSetId::CastleDefault,
			.name = "CastleDefault",
			.entries = {
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::CastleMonster01,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::DemonStriker,
					.spawnPointId = SpawnPointIds::CastleMonster02,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::CastleMonster03,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::DemonExecutioner,
					.spawnPointId = SpawnPointIds::CastleMonster04,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::Tank,
					.spawnPointId = SpawnPointIds::CastleMonster05,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::CastleMonster06,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
			},
		},
		SpawnSetDef{
			.id = SpawnSetId::FinalDefault,
			.name = "FinalDefault",
			.entries = {
				SpawnEntryDef{
					.characterId = CharacterId::Imp,
					.spawnPointId = SpawnPointIds::FinalMonster01,
					.count = 1,
					.type = SpawnConditionType::OnWorldStart,
					.respawnPolicy = RespawnPolicy::None,
					.respawnDelaySec = std::nullopt,
				},
			},
		},
		SpawnSetDef{
			.id = SpawnSetId::PvpDefault,
			.name = "PvpDefault",
			.entries = {},
		},
	};
}

const SpawnSetDef* FindSpawnSetDef(SpawnSetId id) noexcept
{
	for (const SpawnSetDef& def : kSpawnSetDefs)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}
