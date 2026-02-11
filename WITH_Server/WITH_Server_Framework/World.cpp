#include "pch.h"
#include "World.h"

World::World(WorldId id, const WorldDesc& desc, ThreadPool& pool, std::unique_ptr<IWorldImpl> impl)
	: _id(id), _desc(desc), _runtime(pool),
	_impl(std::move(impl))
{
}

void World::Init()
{
	_impl->Build(_runtime);
	_runtime.GraphBuild();
}

void World::Update(const float dT)
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
