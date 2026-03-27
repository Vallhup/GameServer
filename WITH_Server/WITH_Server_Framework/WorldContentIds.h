#pragma once

#include <cstdint>

enum class WorldDefId : uint8_t
{
	None = 0,
	Square = 1,
	Knight_Start = 2,
	Lancer_Start = 3,
	ThirdCharacter_Start = 4,
	Middle = 5,
	Final = 6,
	Pvp = 7
};

enum class SpawnSetId : uint8_t
{
	None = 0,
	SquareDefault = 1,
	KnightStartDefault = 2,
	LancerStartDefault = 3,
	ThirdCharacterStartDefault = 4,
	MiddleDefault = 5,
	FinalDefault = 6,
	PvpDefault = 7,
};