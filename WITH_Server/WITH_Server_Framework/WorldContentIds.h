#pragma once

#include <cstdint>

enum class WorldDefId : uint8_t
{
	None	= 0,
	Square	= 1,
	Start	= 2,
	Middle	= 3,
	Final	= 4,
	Pvp		= 5
};

enum class SpawnSetId : uint8_t
{
	None			= 0,
	SquareDefault	= 1,
	StartDefault	= 2,
	MiddleDefault	= 3,
	FinalDefault	= 4,
	PvpDefault		= 5,
};

using WorldTransferProfileId = uint16_t;
constexpr WorldTransferProfileId InvalidWorldTransferProfileId = 0;

using WorldExecutionModelKey = uint32_t;
constexpr WorldExecutionModelKey InvalidWorldExecutionModelKey = 0;
