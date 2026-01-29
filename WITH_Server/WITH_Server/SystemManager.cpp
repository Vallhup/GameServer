#include "pch.h"
#include "SystemManager.h"
#include "ColliderUpdateSystem.h"
#include "ActionStateSystem.h"
#include "ActionTimeSystem.h"
#include "ActionTransitionSystem.h"
#include "ActionMoveSystem.h"
#include "LocomotionMoveSystem.h"
#include "MovementApplySystem.h"
#include "AnimationCommitSystem.h"
#include "AnimationFrameSystem.h"
#include "AnimationPoseBindSystem.h"
#include "AnimationSelectSystem.h"
#include "ColliderActivationSystem.h"
#include "CombatCollisionCheckSystem.h"
#include "CombatCollisionDedupSystem.h"
#include "CombatCollisionHandlingSystem.h"


void SystemManager::Initalize(ECS& ecs, JobGraph& graph)
{
	//RegisterSystem<ActionStateSystem>(ecs, 1);
	RegisterSystem<ActionTimeSystem>(ecs, 2);
	RegisterSystem<ActionTransitionSystem>(ecs, 3);

	RegisterSystem<ActionMoveSystem>(ecs, 11);
	RegisterSystem<LocomotionMoveSystem>(ecs, 12);
	RegisterSystem<MovementApplySystem>(ecs, 13);

	// TODO : 공간분할

	RegisterSystem<AnimationSelectSystem>(ecs, 21);
	RegisterSystem<AnimationCommitSystem>(ecs, 22);
	RegisterSystem<AnimationFrameSystem>(ecs, 23);
	RegisterSystem<AnimationPoseBindSystem>(ecs, 24);

	RegisterSystem<ColliderUpdateSystem>(ecs, 31);
	RegisterSystem<ColliderActivationSystem>(ecs, 32);
	RegisterSystem<CombatCollisionCheckSystem>(ecs, 33);
	RegisterSystem<CombatCollisionDedupSystem>(ecs, 34);
	RegisterSystem<CombatCollisionHandlingSystem>(ecs, 35);

	// TODO : 시야처리
}

const std::vector<System*> SystemManager::GetSystems() const
{
	std::vector<System*> out;
	for (const auto& system : _systems)
		out.push_back(system.get());

	return out;
}
