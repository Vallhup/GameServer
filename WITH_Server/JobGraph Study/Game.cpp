#include "Game.h"
#include "Instance.h"

#include "EventSystem.h"
#include "CollisionSystem.h"
#include "OutputEventSystem.h"

Game::Game(size_t size)
	: threadPool(size), graph(threadPool)
{
	ecs.systemMng.Initalize(ecs, graph);
	auto systems = ecs.systemMng.GetSystems();
	graph.AutoDependencyBuild(ecs.systemMng.GetSystems(), &_deltaTime);

	ecs.systemMng.RegisterSystem<EventSystem> (ecs, 0);
	ecs.systemMng.RegisterSystem<OutputEventSystem>(ecs, 7);

	graph.Build();
}

void Game::Update(const float dT)
{
	_deltaTime = dT;

	ecs.systemMng.GetSystem<EventSystem>()->Execute(dT);

	graph.Run();

	ecs.systemMng.GetSystem<OutputEventSystem>()->Execute(dT);
}
