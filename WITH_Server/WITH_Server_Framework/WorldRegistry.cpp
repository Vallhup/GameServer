#include "pch.h"
#include "WorldRegistry.h"
#include "World.h"

WorldRegistry::WorldRegistry(uint32 reserve)
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

	slot.gen = gen;
	slot.world = std::make_unique<World>(wId, desc);
}

void WorldRegistry::DetroyWorld(WorldId worldId)
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

	_allocator.Free(worldId);
}

World* WorldRegistry::GetWorld(WorldId worldId)
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

const World* WorldRegistry::GetWorld(WorldId worldId) const
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

void WorldRegistry::EnsureSlotCapacity(uint32 id)
{
	if (id < _worlds.size()) return;
	_worlds.resize(id + 1);
}
