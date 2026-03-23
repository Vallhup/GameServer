#pragma once

#include "NetId.h"
#include "WorldId.h"
#include "Component.h"
#include "EntityType.h"

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
	EntityType entityType;
	Faction faction;
	CharacterType charType;
};

struct AITypeComp : Component
{
	AIArchetypeId aiType;
};

struct ReplicatedTag : TagComponent { };


enum class WorldDirtyType : uint8_t
{
	None = 0,
	Transform = 1 << 0,
	Stat = 1 << 1,
	Animation = 1 << 2
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