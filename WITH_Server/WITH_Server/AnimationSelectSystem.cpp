#include "pch.h"
#include "AnimationSelectSystem.h"
#include "Framework.h"
#include "AnimationType.h"
#include "RepComponent.h"

void AnimationSelectSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	auto& animStates = ecs.GetStorage<AnimationState>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& types = ecs.GetStorage<SpawnTypeComp>();

	for (const auto& [entity, animState] : animStates)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* actionState = actionStates.GetComponent(entity);
		const auto* loco = locos.GetComponent(entity);
		const auto* typeComp = types.GetComponent(entity);
		if (!actionState || !loco || !typeComp) continue;

		auto [next, loop] = AnimationManager::Get()
			.GetAnimationIdForAction(actionState->action, typeComp->type, actionState->attack);
		if (next == AnimationType::None)
		{
			loop = true;
			if (loco->isMoving)
			{
				if (loco->isRun)
					next = AnimationType::Knight_Run;

				else
					next = AnimationType::Knight_Walk;
			}
				
			else
				next = AnimationType::Knight_Idle;
		}

		if (animState.desiredId != next)
		{
#ifdef _DEBUG
			AnimationType prevAnimType = animState.desiredId;
			printf("[Animation] %d -> %d\n",
				ToInt(prevAnimType), ToInt(next));
#endif
			animState.desiredId = next;
			animState.looping = loop;
			animState.speed = 1.0f;

			const auto* netComp = ecs.GetStorage<NetIdComp>().GetComponent(entity);
			if (!netComp) continue;

			Framework::Get().outEventQueue.push(
				OutputEvent::AnimationChanged(netComp->id, next));
		}
	}
}