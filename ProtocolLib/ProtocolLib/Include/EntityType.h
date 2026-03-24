#pragma once

#include <cstdint>
#include <xhash>

enum class EntityType : uint8_t 
{
	None,
	Character,
	Interactable,
	WorldObject,

	Count
};

enum class Faction : uint8_t
{
	Neutral,
	Player,
	Enemy
};

enum class CharacterType : uint8_t
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

enum class AIArchetypeId : uint8_t
{
	None,
	Humanoid,
	NormalMonster,
	FirstBossMonster,
	MidBossMonster,
	FinalBossMonster
};

inline uint32_t ToInt(EntityType type) { return static_cast<uint32_t>(type); }

namespace std 
{
	template<>
	struct hash<EntityType> 
	{
		size_t operator()(const EntityType& type) const noexcept
		{
			return std::hash<uint32_t>()(ToInt(type));
		}
	};
}