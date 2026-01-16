#include "pch.h"
#include "ActionStateSystem.h"
#include "ActionManager.h"

ActionStateSystem::ActionStateSystem(ECS& e, int p) : System(e, p) 
{
}

void ActionStateSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<ActionState>();

	for (const auto& [entity, state] : states)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (state.type != ActionType::None) continue;
		if (auto* intent = ecs.GetStorage<ActionIntent>().GetComponent(entity))
		{
			ActionType nextType = GetNextAction(state, *intent);
			if (nextType == ActionType::None) continue;

			StartAction(entity, &state, nextType);
			ResetActionIntent(intent);
		}
	}
}

ActionType ActionStateSystem::GetNextAction(const ActionState& current, const ActionIntent& intent)
{
	if (intent.parry) return ActionType::Parry;
	if (intent.dodge) return ActionType::Dodge;
	if (intent.attack) return ActionType::Attack;
	if (intent.guard) return ActionType::Guard;
	return ActionType::None;
}

void ActionStateSystem::StartAction(Entity entity, ActionState* state, const ActionType& type)
{
	if (!state) return;

	// TEMP : duration 값 수정 필요(Action별로 다르게)
	state->type = type;
	state->elapsed = 0.0f;

	if (type == ActionType::Guard)
		state->duration = std::numeric_limits<float>::infinity();

	else
	{
		float duration{ 0.0f };
		switch (type) {
		case ActionType::Parry:  duration = 54.0f / 30.566f; break;
		case ActionType::Dodge:	 duration = 50.0f / 30.6122f; break;
		case ActionType::Attack: duration = 40.0f / 30.7692f; break;
		default:                 duration = 0.0f; break;
		}

		state->duration = duration;
	}
		

	if (type == ActionType::Attack || type == ActionType::Dodge)
	{
  		auto* move = ecs.GetStorage<ActionMoveTag>().AddComponent(entity);

 		move->profile = ActionManager::Get().GetActionMoveProfile(type);
		move->segmentIndex = 0;
		move->movedInSegment = 0.0f;

		if (auto* vel = ecs.GetStorage<Velocity>().GetComponent(entity))
		{
			// TEMP : 공격, 회피 방향 정책 수정 필요
			move->dir = vel->lastNonZeroDir;
			move->dirLocked = true;
		}
	}
}

void ActionStateSystem::ResetActionIntent(ActionIntent* intent)
{
	intent->attack = false;
	intent->dodge = false;
	intent->parry = false;
}
