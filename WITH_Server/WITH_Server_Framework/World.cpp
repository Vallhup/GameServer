#include "pch.h"
#include "World.h"
#include "EventRegistry.h"

World::World(WorldId id, const WorldDesc& desc, ThreadPool& pool, std::unique_ptr<IWorldImpl> impl)
	: _id(id), _desc(desc), _runtime(pool, *impl),
	_impl(std::move(impl))
{
	//RegisterWorldEvents(_runtime.Events(), desc);
}

void World::Init()
{
	_impl->SpawnInitial(_runtime);
	_impl->Build(_runtime);
	_runtime.GraphBuild();
}

void World::Update(const double dT)
{
	_runtime.Run(dT);
}

void World::Shutdown()
{
	_impl->OnShutdown(_runtime);
}

Entity World::SpawnPlayer(uint32 connId)
{
	if (_connIds.contains(connId))
		return Entity{};

	Entity e = _impl->SpawnPlayer(_runtime, connId);
	if (!e.IsNull())
		_connIds.insert(connId);

	return e;
};

bool World::TryMakeSnapshot(WorldRuntime& rt, uint32 connId, PlayerSnapshot& out)
{
	return _impl->TryMakeSnapshot(rt, connId, out);
}

bool World::ApplySnapshot(WorldRuntime& rt, uint32 connId, const PlayerSnapshot& snapshot)
{
	return _impl->ApplySnapshot(rt, connId, snapshot);
}

bool World::DespawnPlayer(WorldRuntime& rt, uint32 connId)
{
	const bool result = _impl->DespawnPlayer(rt, connId);
	if(result) _connIds.erase(connId);
	return result;
}

bool World::HasPlayer(uint32 connId)
{
	return _connIds.contains(connId);
}
