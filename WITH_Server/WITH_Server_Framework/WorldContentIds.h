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
	inline constexpr SpawnPointId VillageMonster06 = 207;
	inline constexpr SpawnPointId VillageMonster01A = 211;
	inline constexpr SpawnPointId VillageMonster01B = 212;
	inline constexpr SpawnPointId VillageMonster02A = 213;
	inline constexpr SpawnPointId VillageMonster02B = 214;
	inline constexpr SpawnPointId VillageMonster05A = 215;
	inline constexpr SpawnPointId VillageMonster05B = 216;
	inline constexpr SpawnPointId VillageMonster06A = 217;
	inline constexpr SpawnPointId VillageMonster06B = 218;
	inline constexpr SpawnPointId VillageMonster07 = 219;
	inline constexpr SpawnPointId VillageMonster08 = 220;
	inline constexpr SpawnPointId VillageMonster08A = 221;
	inline constexpr SpawnPointId VillageMonster09 = 222;
	inline constexpr SpawnPointId VillageMonster10 = 223;

	inline constexpr SpawnPointId CastlePlayerStart = 300;
	inline constexpr SpawnPointId CastleMonster01 = 301;
	inline constexpr SpawnPointId CastleMonster02 = 302;
	inline constexpr SpawnPointId CastleMonster03 = 303;
	inline constexpr SpawnPointId CastleMonster04 = 304;
	inline constexpr SpawnPointId CastleMonster05 = 305;
	inline constexpr SpawnPointId CastleMonster06 = 306;
	inline constexpr SpawnPointId CastleMonster07 = 307;
	inline constexpr SpawnPointId CastleMonster08 = 308;
	inline constexpr SpawnPointId CastleMonster09 = 309;
	inline constexpr SpawnPointId CastleMonster10 = 310;
	inline constexpr SpawnPointId CastleMonster01A = 311;
	inline constexpr SpawnPointId CastleMonster01B = 312;
	inline constexpr SpawnPointId CastleMonster03A = 313;
	inline constexpr SpawnPointId CastleMonster03B = 314;
	inline constexpr SpawnPointId CastleMonster11 = 315;
	inline constexpr SpawnPointId CastleMonster12 = 316;
	inline constexpr SpawnPointId CastleMonster07A = 317;
	inline constexpr SpawnPointId CastleMonster07B = 318;
	inline constexpr SpawnPointId CastleMonster12A = 319;
	inline constexpr SpawnPointId CastleMonster12B = 320;

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
