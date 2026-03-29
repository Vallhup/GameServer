#include "pch.h"
#include "WorldRegistry.h"

WorldRegistry::WorldRegistry(IWorldInstanceFactory& factory)
	: _factory(factory)
{
}

const WorldDef* WorldRegistry::FindWorldDef(WorldDefId defId) const
{
	auto it = _defs.find(defId);
	if (it == _defs.end())
		return nullptr;

	return &it->second;
}

void WorldRegistry::RegisterWorldDef(const WorldDef& def)
{
	_defs[def.id] = def;
}

WorldInstance* WorldRegistry::FindWorld(WorldId worldId)
{
	auto it = _worlds.find(worldId);
	if (it == _worlds.end())
		return nullptr;

	return it->second.get();
}

const WorldInstance* WorldRegistry::FindWorld(WorldId worldId) const
{
	auto it = _worlds.find(worldId);
	if (it == _worlds.end())
		return nullptr;

	return it->second.get();
}

WorldInstance* WorldRegistry::CreateWorld(const WorldDef& def, uint64_t instanceKey)
{
	auto impl = _factory.Create(def);
	if (!impl)
		return nullptr;

	const WorldId worldId = _idAllocator.Allocate();
	if (!worldId.IsValid())
		return nullptr;

	WorldInstanceCreateParams params;
	params.identity.id = worldId;
	params.identity.defId = def.id;
	params.identity.instanceKey = instanceKey;
	params.def = &def;
	params.impl = std::move(impl);

	// TODO
	// params.executionModel = _factory.CreateExecutionModel(def);

	auto instance = std::make_unique<WorldInstance>(std::move(params));
	WorldInstance* raw = instance.get();

	_worlds.try_emplace(worldId, std::move(instance));
	return raw;
}

bool WorldRegistry::DestroyWorld(WorldId worldId)
{
	auto it = _worlds.find(worldId);
	if (it == _worlds.end())
		return false;

	it->second->Shutdown();
	_worlds.erase(it);

	_idAllocator.Free(worldId);
	return true;
}

bool WorldRegistry::IsAlive(WorldId worldId) const
{
	return _idAllocator.IsAlive(worldId);
}

void WorldRegistry::Clear()
{
	for (auto& [_, world] : _worlds)
	{
		if (world)
			world->Shutdown();
	}

	_worlds.clear();
	_defs.clear();
	_idAllocator.Clear();
}