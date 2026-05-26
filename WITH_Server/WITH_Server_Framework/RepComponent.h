#pragma once

#include "NetId.h"
#include "WorldId.h"
#include "Component.h"
#include "EntityId.h"

struct NetIdComp : Component
{
	NetId id;
};

struct WorldIdComp : Component
{
	WorldId id;
};

struct SpawnTypeComp : Component
{
	CharacterId characterId;
};

struct AITypeComp : Component
{
	AIArchetype aiType;
	uint64_t aiProfileId{ 0 };
};

struct ReplicatedTag : TagComponent { };


enum class WorldDirtyType : uint8_t
{
	None = 0,
	Transform = 1 << 0,
	Stat = 1 << 1,
	Animation = 1 << 2,
	Inventory = 1 << 3,
	MonsterCombatState = 1 << 4
};

class DirtyFlagsComp : public Component {
public:
	inline void MarkDirty(WorldDirtyType type)
	{
		flags |= static_cast<uint8_t>(type);
	}

	inline void Clear()
	{
		flags = 0;
	}

	inline bool IsDirty(WorldDirtyType type) const
	{
		return (flags & static_cast<uint8_t>(type)) != 0;
	}

	inline bool AnyDirty() const
	{
		return flags != 0;
	}

private:
	uint8_t flags{ 0 };
};
