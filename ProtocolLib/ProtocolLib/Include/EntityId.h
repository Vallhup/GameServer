#pragma once

#include <cstdint>
#include <xhash>

enum class CharacterId : uint8_t
{
	None,

	Knight,
	Lancer,
	/* 3번째 캐릭터 */

	Imp,
	/* 2번째 잡몹 */
	/* 3번째 잡몹 */

	DemonWarrior,
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