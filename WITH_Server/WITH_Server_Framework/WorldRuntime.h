#pragma once

#include "ECS.h"
#include "JobGraph.h"
#include "EventRegistry.h"
#include "RepComponent.h"
#include "SystemScheduler.h"

class IWorldImpl;
class ThreadPool;

class WorldRuntime {
public:
	WorldRuntime(ThreadPool& pool, IWorldImpl& impl);

	void GraphBuild();
	void Run(const double dT);

	Entity SpawnPlayer(uint32 connId);
	void MarkDirty(Entity entity, WorldDirtyType type);

	ECS& GetECS() { return _ecs; }
	const ECS& GetECS() const { return _ecs; }

	EventRegistry& Events() { return _events; }
	const EventRegistry& Events() const { return _events; }

	std::vector<Entity>& DirtyEntities() { return _dirtyEntities; }
	const std::vector<Entity>& DirtyEntities() const { return _dirtyEntities; }

	// void Commit();

private:
	void RunPre(const double dT);
	void RunPost(const double dT);

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

	// CommandBuffer _commandBuffer;
};

