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
	_impl->ApplyInbox(_runtime, dT);
	_impl->Execute(_runtime, dT);
	_impl->BuildOutbox(_runtime, dT);
	_impl->FlushOutbox(_runtime, dT);

	// _runtime.Commit();
}

void World::Shutdown()
{
	_impl->OnShutdown(_runtime);
}

Entity World::SpawnPlayer(uint32 connId)
{
	return _impl->SpawnPlayer(_runtime, connId);
};