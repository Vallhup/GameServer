#pragma once

#include <cstdint>
#include <xhash>

enum class CharacterId : uint8_t
{
	None,

	Knight,
	Lancer,
	Vanguard,

	Imp,
	DemonStriker,
	DemonExecutioner,

	BigDemonWarrior,
	Tank,
	FinalBoss
};

enum class Faction : uint8_t
{
	None,
	Neutral,
	Player,
	Enemy
};

enum class AIArchetype : uint8_t
{
	None,
	Humanoid,
	NormalMonster,
	FirstBossMonster,
	MidBossMonster,
	FinalBossMonster
};