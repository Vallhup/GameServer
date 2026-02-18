#pragma once

#include "WorldIdAllocator.h"

class IWorld;
struct WorldDesc;
class IWorldFactory;
class ThreadPool;

class WorldRegistry {
public:
	explicit WorldRegistry(uint32 reserve, ThreadPool& pool, IWorldFactory& factory);

	WorldId CreateWorld(const WorldDesc& desc);
	void DestroyWorld(WorldId worldId);

	IWorld* GetWorld(WorldId worldId);
	const IWorld* GetWorld(WorldId worldId) const;

	bool IsAlive(WorldId worldId) const { return _allocator.IsAlive(worldId); }

private:
	struct WorldSlot
	{
		uint32 gen{ 0 };
		std::unique_ptr<IWorld> world{ nullptr };
	};

	void EnsureSlotCapacity(uint32 id);

	ThreadPool& _threadPool;
	IWorldFactory& _worldFactory;

	WorldIdAllocator _allocator;
	std::vector<WorldSlot> _worlds;
};

