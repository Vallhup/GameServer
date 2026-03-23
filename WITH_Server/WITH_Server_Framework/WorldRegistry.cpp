#include "pch.h"
#include "WorldRegistry.h"
#include "World.h"
#include "ThreadPool.h"
#include "WorldFactory.h"

WorldRegistry::WorldRegistry(uint32 reserve, ThreadPool& pool, IWorldFactory& factory)
	: _threadPool(pool), _worldFactory(factory)
{
	_worlds.reserve(reserve);
	_worlds.push_back(WorldSlot{});
}

WorldId WorldRegistry::CreateWorld(const WorldDesc& desc)
{
	WorldId wId = _allocator.Allocate();
	const uint32 id = wId.GetId();
	const uint32 gen = wId.GetGen();

	EnsureSlotCapacity(id);

	WorldSlot& slot = _worlds[id];
	assert(slot.world == nullptr);
	assert(slot.gen == 0 || slot.gen != gen);

	slot.gen = gen;

	try
	{
		if (auto impl = _worldFactory.CreateImpl(desc))
		{
			auto world = std::make_unique<World>(wId, desc, _threadPool, std::move(impl));
			world->Init();

			slot.world = std::move(world);
			return wId;
		}

		else
		{
			RollbackCreate(wId);
			return WorldId::Invalid();
		}
	}
	catch (...)
	{
		RollbackCreate(wId);
		throw;
	}
}

void WorldRegistry::DestroyWorld(WorldId worldId)
{
	if (!worldId.IsValid()) return;

	const uint32 id = worldId.GetId();
	if (id == 0 || id >= _worlds.size()) return;

	WorldSlot& slot = _worlds[id];

	if (slot.gen != worldId.GetGen()) return;

	if (slot.world)
	{
		slot.world->Shutdown();
		slot.world.reset();
	}
	slot.gen = 0;

	_allocator.Free(worldId);
}

IWorld* WorldRegistry::GetWorld(WorldId worldId)
{
	if (!worldId.IsValid()) return nullptr;

	const uint32 id = worldId.GetId();
	if (id == 0 || id >= _worlds.size())
		return nullptr;

	WorldSlot& slot = _worlds[id];
	if (slot.gen != worldId.GetGen())
		return nullptr;

	return slot.world.get();
}

const IWorld* WorldRegistry::GetWorld(WorldId worldId) const
{
	if (!worldId.IsValid()) return nullptr;

	const uint32 id = worldId.GetId();
	if (id == 0 || id >= _worlds.size())
		return nullptr;

	const WorldSlot& slot = _worlds[id];
	if (slot.gen != worldId.GetGen())
		return nullptr;

	return slot.world.get();
}

void WorldRegistry::Clear()
{
	for (WorldSlot& slot : _worlds)
	{
		if (slot.world)
		{
			slot.world->Shutdown();
			slot.world.reset();
		}
	}

	_worlds.clear();
	_allocator.Clear();
}

void WorldRegistry::EnsureSlotCapacity(uint32 id)
{
	if (id < _worlds.size()) return;
	_worlds.resize(id + 1);
}

void WorldRegistry::RollbackCreate(WorldId worldId)
{
	if (!worldId.IsValid()) return;

	const uint32_t id = worldId.GetId();
	if (id == 0 || id >= _worlds.size())
		return;

	WorldSlot& slot = _worlds[id];
	slot.world.reset();
	slot.gen = 0;

	_allocator.Free(worldId);
}

