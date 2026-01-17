#include "pch.h"
#include "AnimationSelectSystem.h"

AnimationSelectSystem::AnimationSelectSystem(ECS& e, int p) 
	: System(e, p)
{
	_actionToAnimationMap[ActionType::Attack]	= { AnimationId::Knight_Attack, false };
	_actionToAnimationMap[ActionType::Dodge]	= { AnimationId::Knight_Dodge, false };
	_actionToAnimationMap[ActionType::Parry]	= { AnimationId::Knight_Parry, false };
	_actionToAnimationMap[ActionType::Guard]	= { AnimationId::Knight_Guard, true };
	_actionToAnimationMap[ActionType::Stun]		= { AnimationId::Knight_Stun, false };
	_actionToAnimationMap[ActionType::Hit]		= { AnimationId::Knight_Hit, false };
	_actionToAnimationMap[ActionType::Dead]		= { AnimationId::Knight_Dead, false };
}

void AnimationSelectSystem::Execute(const float dT)
{
	auto& animStates = ecs.GetStorage<AnimationState>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& locos = ecs.GetStorage<LocomotionState>();

	for (const auto& [entity, animState] : animStates)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* actionState = actionStates.GetComponent(entity);
		const auto* loco = locos.GetComponent(entity);
		if (!actionState || !loco) continue;

		auto [next, loop] = GetAnimationIdForAction(actionState->type);
		if (next == AnimationId::None)
		{
			loop = true;
			if (loco->isMoving)
				next = AnimationId::Knight_Walk;

			else
				next = AnimationId::Knight_Idle;
		}

		animState.desiredId = next;
		animState.looping = loop;
		animState.speed = 1.0f;
	}
}

std::pair<AnimationId, bool> AnimationSelectSystem::GetAnimationIdForAction(ActionType action) const
{
	auto it = _actionToAnimationMap.find(action);
	if (it != _actionToAnimationMap.end()) return it->second;
	return { AnimationId::None, false };
}
