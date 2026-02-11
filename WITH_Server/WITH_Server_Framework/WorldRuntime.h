#pragma once

#include "ECS.h"
#include "JobGraph.h"

class WorldRuntime {
public:
	WorldRuntime(ThreadPool& pool);

	void GraphBuild();

	// void Commit();

private:
	ECS _ecs;
	JobGraph _graph;
	ThreadPool& _threadPool;

	// CommandBuffer _commandBuffer;
};

