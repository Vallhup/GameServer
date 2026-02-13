#include "pch.h"
#include "WorldRuntime.h"

WorldRuntime::WorldRuntime(ThreadPool& pool)
	: _graph(pool), _threadPool(pool), _graphBuilt(false)
{
}

void WorldRuntime::GraphBuild()
{
	auto systemsForGraph = _ecs._systemMng.GetSystems(SystemPhase::Graph);

	_graph.AutoDependencyBuild(systemsForGraph, &_deltaTime);
	_graph.Build();

	_graphBuilt = true;
}

void WorldRuntime::Run(const float dT)
{
	assert(_graphBuilt && "WorldRuntime::GraphBuild must be called before Run.");

	_deltaTime = dT;

	RunPre(dT);
	
	_graph.Run();

	RunPost(dT);
}

void WorldRuntime::RunPre(const float dT)
{
	for (System* s : _ecs._systemMng.GetSystems(SystemPhase::Pre))
		s->Execute(dT);
}

void WorldRuntime::RunPost(const float dT)
{
	for (System* s : _ecs._systemMng.GetSystems(SystemPhase::Post))
		s->Execute(dT);
}
