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

	void BuildGraph();
	void Run(const double dT);

	Entity SpawnPlayer(uint32 connId);
	void MarkDirty(Entity entity, WorldDirtyType type);

	template<CompT T>
	void ImmediateAddComponent(Entity e)
	{
		_ecs.GetStorage<T>().EmplaceComponent(e);
	}

	template<CompT T>
	void ImmediateAddComponent(Entity e, T&& value)
	{
		_ecs.AddComponentImmediate<T>(e, std::move(value));
	}

	template<CompT T>
	void DeferredAddComponent(Entity e)
	{
		_commandBuffer.Enqueue(
			[e](WorldRuntime& rt)
			{
				auto& storage = rt.GetECS().GetStorage<T>();
				T* comp = storage.GetComponent(e);
				if (!comp)
					comp = storage.EmplaceComponent(e);
			}
		);
	}

	template<CompT T, typename Fn>
	void DeferredUpsertComponent(Entity e, Fn&& fn)
	{
		_commandBuffer.Enqueue(
			[e, func = std::forward<Fn>(fn)](WorldRuntime& rt)
			{
				auto& storage = rt.GetECS().GetStorage<T>();
				T* comp = storage.GetComponent(e);
				if (!comp)
					comp = storage.EmplaceComponent(e);

				func(*comp);
			}
		);
	}

	template<typename T>
	void DeferredAssignComponent(Entity e, T&& value)
	{
		using U = std::decay_t<T>;

		_commandBuffer.Enqueue(
			[e, value = U(std::forward<T>(value))](WorldRuntime& rt) mutable
			{
				rt.GetECS().AddComponentImmediate<T>(e, std::move(value));
			}
		);
	}

	template<typename T>
	void DeferredRemoveComponent(Entity e)
	{
		_commandBuffer.Enqueue(
			[e](WorldRuntime& rt)
			{
				rt.GetECS().RemoveComponentImmediate<T>(e);
			}
		);
	}

	void DeferredCreateEntity();
	void DeferredDestroyEntity(Entity e);
	void DeferredMarkDirty(Entity e, WorldDirtyType dirtyType);

	template<SysT T, typename... Args>
	T* AddSystem(SystemPhase phase, Args&&... args)
	{
		return _systemMng.RegisterSystem<T>(phase, std::forward<Args>(args)...);
	}

	template<SysT T>
	T* GetSystem(SystemPhase phase)
	{
		return _systemMng.GetSystem<T>(phase);
	}

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

	ECS _ecs;
	SystemManager _systemMng;
	SystemScheduler _scheduler;
	CompiledSystemSchedule _compiledGraphSchedule;
	CommandBuffer _commandBuffer;

	//JobGraph _graph;
	ThreadPool& _threadPool;

	EventRegistry _events;
	IWorldImpl& _impl;

	double _deltaTime;
	bool _graphBuilt;
	
	std::vector<Entity> _dirtyEntities;
};