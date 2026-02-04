#include "pch.h"
#include "Game.h"
#include "Instance.h"

#include "EventSystem.h"
#include "OutputEventSystem.h"

#include "ActionTransitionSystem.h"
#include "AnimationSelectSystem.h"

Game::Game(size_t size)
	: threadPool(size), graph(threadPool)
{
	ecs.systemMng.Initalize(ecs, graph);
	graph.AutoDependencyBuild(ecs.systemMng.GetSystems(), &_deltaTime);

	auto* actionTransSystem = ecs.systemMng.GetSystem<ActionTransitionSystem>();
	auto* animSelectSystem = ecs.systemMng.GetSystem<AnimationSelectSystem>();

	graph.AddManualDependency(actionTransSystem, animSelectSystem);

	ecs.systemMng.RegisterSystem<EventSystem> (ecs, 0);
	ecs.systemMng.RegisterSystem<OutputEventSystem>(ecs, 100);

	graph.Build();

	Entity boss = ecs.entityMng.Create();
	boss.type = EntityType::Final_Boss;

	auto* transform = ecs.GetStorage<Transform>().AddComponent(boss);
	transform->position =
	{ 22.0f, MapCollisionManager::Get().SampleHeightAt(22.0f, 22.0f), 22.0f };
	ecs.GetStorage<AIState>().AddComponent(boss);
	ecs.GetStorage<AIThinkState>().AddComponent(boss);
	ecs.GetStorage<ActionState>().AddComponent(boss);
	ecs.GetStorage<AttackData>().AddComponent(boss);
	ecs.GetStorage<Health>().AddComponent(boss);
	ecs.GetStorage<AttackState>().AddComponent(boss);
}

void Game::Update(const float dT)
{
	_deltaTime = dT;

	ecs.systemMng.GetSystem<EventSystem>()->Execute(dT);

	graph.Run();

	ecs.systemMng.GetSystem<OutputEventSystem>()->Execute(dT);
}
