#include "pch.h"
#include "WorldRuntime.h"
#include "World.h"

namespace
{
	std::vector<SystemScheduleDesc> BuildGraphScheduleDescs(std::span<System*> systems)
	{
		std::vector<SystemScheduleDesc> descs;
		descs.reserve(systems.size());

		for (size_t i = 0; i < systems.size(); ++i)
		{
			System* s = systems[i];
			if (!s)
				throw std::runtime_error("Null system in graph-phase system list.");

			const SystemMeta& meta = s->Meta();

			SystemScheduleDesc desc;
			desc.system = s;
			desc.meta = &meta;
			desc.registrationOrder = static_cast<uint32_t>(i);

			descs.push_back(desc);
		}

		return descs;
	}
}

WorldRuntime::WorldRuntime(ThreadPool& pool, IWorldImpl& impl)
	: /*_graph(pool),*/ _threadPool(pool), _impl(impl),
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

void WorldRuntime::DeferredDestroy(Entity e)
{
	_commandBuffer.Enqueue(
		[e](WorldRuntime& rt)
		{
			rt.GetECS().DestroyEntity(e);
		}
	);
}

void WorldRuntime::DeferredMarkDirty(Entity e, WorldDirtyType dirtyType)
{
	_commandBuffer.Enqueue(
		[e, dirtyType](WorldRuntime& rt)
		{
			rt.MarkDirty(e, dirtyType);
		}
	);
}

void WorldRuntime::GraphBuild()
{
	auto systemsForGraph = _ecs._systemMng.GetSystems(SystemPhase::Graph);
	
	{
		auto descs = BuildGraphScheduleDescs(systemsForGraph);
		_compiledGraphSchedule = _scheduler.Compile(descs);
	}

	/*_graph.AutoDependencyBuild(systemsForGraph, &_deltaTime);
	_graph.Build();*/

	_graphBuilt = true;
}

void WorldRuntime::Run(const double dT)
{
	assert(_graphBuilt && "WorldRuntime::GraphBuild must be called before Run.");

	_deltaTime = dT;

	try
	{
		RunPre(dT);
		RunGraph(dT);
		RunPost(dT);

		_commandBuffer.Commit(*this);
	}
	catch (...)
	{
		_commandBuffer.Clear();
		throw;
	}
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

void WorldRuntime::RunGraph(const double dT)
{
	//_graph.Run();
	_scheduler.Execute(_compiledGraphSchedule, _threadPool, dT);
}
