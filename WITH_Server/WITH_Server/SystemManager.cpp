#include "pch.h"
#include "SystemManager.h"
#include "AnimationStateSystem.h"
#include "AnimationTimeSystem.h"
#include "AnimationRefSystem.h"
#include "ColliderUpdateSystem.h"
#include "CollisionSystem.h"
#include "ActionStateSystem.h"
#include "ActionTimeSystem.h"
#include "ActionTransitionSystem.h"
#include "HitResolveSystem.h"
#include "HitApplySystem.h"
#include "ActionMoveSystem.h"
#include "LocomotionMoveSystem.h"
#include "MovementApplySystem.h"

void SystemManager::Initalize(ECS& ecs, JobGraph& graph)
{
	RegisterSystem<ActionStateSystem>(ecs, 0);
	RegisterSystem<ActionTimeSystem>(ecs, 1);
	RegisterSystem<ActionTransitionSystem>(ecs, 2);

	RegisterSystem<ActionMoveSystem>(ecs, 3);
	RegisterSystem<LocomotionMoveSystem>(ecs, 4);
	RegisterSystem<MovementApplySystem>(ecs, 5);

	RegisterSystem<AnimationStateSystem>(ecs, 6);
	RegisterSystem<AnimationTimeSystem>(ecs, 7);
	RegisterSystem<AnimationRefSystem>(ecs, 8);

	RegisterSystem<ColliderUpdateSystem>(ecs, 9);
	RegisterSystem<CollisionSystem>(ecs, 10);

	RegisterSystem<HitResolveSystem>(ecs, 11);
	RegisterSystem<HitApplySystem>(ecs, 12);
}

const std::vector<System*> SystemManager::GetSystems() const
{
	std::vector<System*> out;
	for (const auto& system : _systems)
		out.push_back(system.get());

	return out;
}
