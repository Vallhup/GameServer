#include "pch.h"
#include "WorldRuntime.h"
#include "World.h"

WorldRuntime::WorldRuntime(ThreadPool& pool, IWorldImpl& impl)
	: _graph(pool), _threadPool(pool), _impl(impl),
	_deltaTime(0.0), _graphBuilt(false)
{
}

Entity WorldRuntime::SpawnPlayer(uint32 connId)
{
	return _impl.SpawnPlayer(*this, connId);
}

void WorldRuntime::MarkDirty(Entity entity, WorldDirtyType type)
{
	auto* dirty = _ecs.GetStorage<DirtyFlagsComp>().GetComponent(entity);
	if (dirty)
	{
		const bool wasClean = !dirty->AnyDirty();
		dirty->MarkDirty(type);

		if (wasClean)
			_dirtyEntities.push_back(entity);
	}
}

void WorldRuntime::GraphBuild()
{
	auto systemsForGraph = _ecs._systemMng.GetSystems(SystemPhase::Graph);

	_graph.AutoDependencyBuild(systemsForGraph, &_deltaTime);
	_graph.Build();

	_graphBuilt = true;
}

void WorldRuntime::Run(const double dT)
{
	assert(_graphBuilt && "WorldRuntime::GraphBuild must be called before Run.");

	_deltaTime = dT;


	RunPre(dT);
	_graph.Run();
	RunPost(dT);
}

void WorldRuntime::RunPre(const double dT)
{
	for (System* s : _ecs._systemMng.GetSystems(SystemPhase::Pre))
		s->Execute(dT);
}

void WorldRuntime::RunPost(const double dT)
{
	for (System* s : _ecs._systemMng.GetSystems(SystemPhase::Post))
		s->Execute(dT);
}