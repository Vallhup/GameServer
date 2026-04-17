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
	PlazaDefault	= 1,
	VillageDefault	= 2,
	CastleDefault	= 3,
	FinalDefault	= 4,
	PvpDefault		= 5,
};

using WorldTransferProfileId = uint16_t;
constexpr WorldTransferProfileId InvalidWorldTransferProfileId = 0;
constexpr WorldTransferProfileId PlayerCharacterWorldTransferProfileId = 1;

using WorldExecutionModelKey = uint32_t;
constexpr WorldExecutionModelKey InvalidWorldExecutionModelKey = 0;
