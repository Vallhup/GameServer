#pragma once

#include "ECS.h"
#include "JobGraph.h"
#include "EventRegistry.h"
#include "RepComponent.h"
#include "SystemScheduler.h"
#include "CommandBuffer.h"

class IWorldImpl;
class ThreadPool;

class WorldRuntime {
public:
	WorldRuntime(ThreadPool& pool, IWorldImpl& impl);

	void GraphBuild();
	void Run(const double dT);

	Entity SpawnPlayer(uint32 connId);
	void MarkDirty(Entity entity, WorldDirtyType type);

	template<typename T>
	void DeferredAssign(Entity e, T&& value)
	{
		using U = std::decay_t<T>;

		_commandBuffer.Enqueue(
			[e, value = U(std::forward<T>(value))](WorldRuntime& rt) mutable
			{
				rt.GetECS().GetStorage<U>().
					AddOrAssignComponent(e, std::move(value));
			}
		);
	}

	template<typename T>
	void DeferredRemove(Entity e)
	{
		_commandBuffer.Enqueue(
			[e](WorldRuntime& rt)
			{
				rt.GetECS().GetStorage<T>().RemoveComponent(e);
			}
		);
	}

	void DeferredDestroy(Entity e);
	void DeferredMarkDirty(Entity e, WorldDirtyType dirtyType);

	ECS& GetECS() { return _ecs; }
	const ECS& GetECS() const { return _ecs; }

	EventRegistry& Events() { return _events; }
	const EventRegistry& Events() const { return _events; }

	CommandBuffer& Commands() { return _commandBuffer; }
	const CommandBuffer& Commands() const { return _commandBuffer; }

	std::vector<Entity>& DirtyEntities() { return _dirtyEntities; }
	const std::vector<Entity>& DirtyEntities() const { return _dirtyEntities; }

private:
	void RunPre(const double dT);
	void RunPost(const double dT);
	void RunGraph(const double dT);

	friend class ECS;

	ECS _ecs;
	//JobGraph _graph;
	ThreadPool& _threadPool;

	SystemScheduler _scheduler;
	CompiledSystemSchedule _compiledGraphSchedule;

	EventRegistry _events;
	IWorldImpl& _impl;

	double _deltaTime;
	bool _graphBuilt;
	
	std::vector<Entity> _dirtyEntities;
	CommandBuffer _commandBuffer;
};

