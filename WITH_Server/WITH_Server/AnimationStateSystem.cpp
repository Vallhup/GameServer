#include "pch.h"
#include "AnimationStateSystem.h"

void AnimationStateSystem::Execute(const float dT)
{
	auto& actions = ecs.GetStorage<ActionState>();
	auto& animations = ecs.GetStorage<AnimationState>();
	auto& locos = ecs.GetStorage<LocomotionState>();

	for (const auto& [entity, anim] : animations)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* action = actions.GetComponent(entity);
		auto* loco = locos.GetComponent(entity);
		if (!action or !loco) continue;

		AnimationId desired{ anim.id };

		// TODO : 상태에 따라 애니메이션 상태도 변화
		if (action->type == ActionType::None)
		{
			if (loco->isMoving)
				desired = AnimationId::Knight_Walk;

			else
				desired = AnimationId::Knight_Idle;
		}

		else
		{
			// TEMP : 추후 ActionState <-> Animation 매핑 테이블 필요
			switch (action->type) {
			case ActionType::Attack:
			{
				desired = AnimationId::Knight_Attack;
				break;
			}
			case ActionType::Dodge:
			{
				desired = AnimationId::Knight_Dodge;
				break;
			}
			case ActionType::Parry:
			{
				desired = AnimationId::Knight_Parry;
				break;
			}
			case ActionType::Hit:
			{
				desired = AnimationId::Knight_Hit;
				break;
			}
			case ActionType::Dead:
			{
				desired = AnimationId::Knight_Dead;
				break;
			}
			}
		}

		if (desired != anim.id)
			ChangeAnimation(entity, desired);
	}
}

void AnimationStateSystem::ChangeAnimation(Entity e, AnimationId id)
{
	auto* state = ecs.GetStorage<AnimationState>().GetComponent(e);
	auto* ref = ecs.GetStorage<AnimationRef>().GetComponent(e);
	if (!state or !ref) return;
	if (state->id == id) return;

	state->id = id;
	ref->anim = AnimationManager::Get().GetAnimation(id);

	switch (id) {
	case AnimationId::Knight_Idle:
	case AnimationId::Knight_Walk:
	case AnimationId::Knight_Run:
		state->looping = true;
		break;

	case AnimationId::Knight_Attack:
	case AnimationId::Knight_Dodge:
	case AnimationId::Knight_Parry:
	case AnimationId::Knight_Hit:
	case AnimationId::Knight_Dead:
		state->looping = false;
		break;
	}
}
