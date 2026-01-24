#include "pch.h"
#include "AnimationSelectSystem.h"
#include "Framework.h"
#include "AnimationType.h"

AnimationSelectSystem::AnimationSelectSystem(ECS& e, int p) 
	: System(e, p)
{
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

		auto [next, loop] = AnimationManager::Get()
			.GetAnimationIdForAction(actionState->type);
		if (next == AnimationType::None)
		{
			loop = true;
			if (loco->isMoving)
				next = AnimationType::Knight_Walk;

			else
				next = AnimationType::Knight_Idle;
		}

		if (animState.desiredId != next)
		{
			AnimationType prevAnimType = animState.desiredId;
			animState.desiredId = next;
			animState.looping = loop;
			animState.speed = 1.0f;

			Framework::Get().outEventQueue.push(
				OutputEvent::AnimationChanged(entity, prevAnimType, next));

#ifdef _DEBUG
			printf("[Animation] %d -> %d\n",
				ToInt(prevAnimType), ToInt(next));
#endif
		}
	}
}