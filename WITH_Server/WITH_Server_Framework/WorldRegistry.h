#pragma once

#include "WorldIdAllocator.h"

class World;
struct WorldDesc;

class WorldRegistry {
public:
	explicit WorldRegistry(uint32 reserve = 256);

	WorldId CreateWorld(const WorldDesc& desc);
	void DestroyWorld(WorldId worldId);

	World* GetWorld(WorldId worldId);
	const World* GetWorld(WorldId worldId) const;

	bool IsAlive(WorldId worldId) const { return _allocator.IsAlive(worldId); }

private:
	struct WorldSlot
	{
		uint16 gen{ 0 };
		std::unique_ptr<World> world{ nullptr };
	};

	void EnsureSlotCapacity(uint32 id);

	WorldIdAllocator _allocator;
	std::vector<WorldSlot> _worlds;
};

