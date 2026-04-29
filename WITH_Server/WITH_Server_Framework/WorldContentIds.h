#pragma once

#include <cstdint>

enum class WorldDefId : uint8_t
{
	None	= 0,
	Plaza	= 1,
	Village	= 2,
	Castle	= 3,
	Final	= 4,
	Pvp		= 5
};

enum class SpawnSetId : uint8_t
{
	None			= 0,
	CastleDefault	= 1,
	FinalDefault	= 2,
	PlazaDefault	= 3,
	PvpDefault		= 4,
	VillageDefault	= 5,
};

using SpawnPointId = uint16_t;

namespace SpawnPointIds
{
	inline constexpr SpawnPointId None = 0;

	inline constexpr SpawnPointId PlazaPlayerStart = 100;
	inline constexpr SpawnPointId PlazaMonster01 = 101;

	inline constexpr SpawnPointId VillagePlayerStart = 200;
	inline constexpr SpawnPointId VillageMonster01 = 201;
	inline constexpr SpawnPointId VillageMonster02 = 202;
	inline constexpr SpawnPointId VillageMonster03 = 203;
	inline constexpr SpawnPointId VillageMonster04 = 204;
	inline constexpr SpawnPointId VillageMonster05 = 205;
	inline constexpr SpawnPointId VillageBossMonster01 = 206;

	inline constexpr SpawnPointId CastlePlayerStart = 300;
	inline constexpr SpawnPointId CastleMonster01 = 301;
	inline constexpr SpawnPointId CastleMonster02 = 302;
	inline constexpr SpawnPointId CastleMonster03 = 303;
	inline constexpr SpawnPointId CastleMonster04 = 304;
	inline constexpr SpawnPointId CastleMonster05 = 305;
	inline constexpr SpawnPointId CastleMonster06 = 306;

	inline constexpr SpawnPointId FinalPlayerStart = 400;
	inline constexpr SpawnPointId FinalMonster01 = 401;

	inline constexpr SpawnPointId PvpPlayerStartA = 500;
	inline constexpr SpawnPointId PvpPlayerStartB = 501;
}

using WorldTransferProfileId = uint16_t;
constexpr WorldTransferProfileId InvalidWorldTransferProfileId = 0;
constexpr WorldTransferProfileId PlayerCharacterWorldTransferProfileId = 1;

using WorldExecutionModelKey = uint32_t;
constexpr WorldExecutionModelKey InvalidWorldExecutionModelKey = 0;
