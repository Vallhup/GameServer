#pragma once

#include "ECS.h"
#include "JobGraph.h"

class WorldRuntime {
public:
	WorldRuntime(ThreadPool& pool);

	void GraphBuild();
	void Run(const float dT);

	ECS& GetECS() { return _ecs; }
	const ECS& GetECS() const { return _ecs; }

	// void Commit();

private:
	void RunPre(const float dT);
	void RunPost(const float dT);

	friend class ECS;

	ECS _ecs;
	JobGraph _graph;
	ThreadPool& _threadPool;

	float _deltaTime;
	bool _graphBuilt;

	// CommandBuffer _commandBuffer;
};

