#include "SystemManager.h"
#include "EventSystem.h"
#include "MovementSystem.h"
#include "AnimationStateSystem.h"
#include "AnimationTimeSystem.h"
#include "AnimationRefSystem.h"
#include "ColliderUpdateSystem.h"
#include "CollisionSystem.h"
#include "OutputEventSystem.h"

void SystemManager::Initalize(ECS& ecs, JobGraph& graph)
{
	//RegisterSystem<EventSystem>(ecs, 0);
	RegisterSystem<MovementSystem>(ecs, 1);
	RegisterSystem<AnimationStateSystem>(ecs, 2);
	RegisterSystem<AnimationTimeSystem>(ecs, 3);
	RegisterSystem<AnimationRefSystem>(ecs, 4);
	RegisterSystem<ColliderUpdateSystem>(ecs, 5);
	RegisterSystem<CollisionSystem>(ecs, 6);
	//RegisterSystem<OutputEventSystem>(ecs, 7);
}

const std::vector<System*> SystemManager::GetSystems() const
{
	std::vector<System*> out;
	for (const auto& system : _systems)
		out.push_back(system.get());

	return out;
}
