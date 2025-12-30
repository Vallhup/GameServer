#include "SystemManager.h"
#include "MovementSystem.h"
#include "AnimationStateSystem.h"
#include "AnimationTimeSystem.h"
#include "AnimationRefSystem.h"
#include "ColliderUpdateSystem.h"
#include "CollisionSystem.h"
#include "ActionStateSystem.h"
#include "ActionTimeSystem.h"
#include "ActionTransitionSystem.h"

void SystemManager::Initalize(ECS& ecs, JobGraph& graph)
{
	RegisterSystem<ActionStateSystem>(ecs, 0);
	RegisterSystem<ActionTimeSystem>(ecs, 1);
	RegisterSystem<ActionTransitionSystem>(ecs, 2);
	RegisterSystem<MovementSystem>(ecs, 3);
	RegisterSystem<AnimationStateSystem>(ecs, 4);
	RegisterSystem<AnimationTimeSystem>(ecs, 5);
	RegisterSystem<AnimationRefSystem>(ecs, 6);
	RegisterSystem<ColliderUpdateSystem>(ecs, 7);
	RegisterSystem<CollisionSystem>(ecs, 8);
}

const std::vector<System*> SystemManager::GetSystems() const
{
	std::vector<System*> out;
	for (const auto& system : _systems)
		out.push_back(system.get());

	return out;
}
