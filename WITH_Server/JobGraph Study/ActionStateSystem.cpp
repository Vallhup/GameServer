#include "pch.h"
#include "ActionStateSystem.h"

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

			StartAction(&state, nextType);
			ResetActionIntent(intent);
		}
	}
}

std::vector<std::type_index> ActionStateSystem::WriteComponents() const
{
	return { typeid(ActionIntent), typeid(ActionState) };
}

ActionType ActionStateSystem::GetNextAction(const ActionState& current, const ActionIntent& intent)
{
	// TODO : 우선순위 정의 필요
	// Guard의 우선순위에 따라 구현 변화 가능
	if (intent.parry) return ActionType::Parry;
	if (intent.dodge) return ActionType::Dodge;
	if (intent.attack) return ActionType::Attack;
	if (intent.guard) return ActionType::Guard;
	return ActionType::None;
}

bool ActionStateSystem::StartAction(ActionState* state, const ActionType& type)
{
	if (!state) return false;

	// TEMP : duration 값 수정 필요(Action별로 다르게)
	state->type = type;
	state->elapsed = 0.0f;

	if(type == ActionType::Guard)
		state->duration = std::numeric_limits<float>::infinity();

	else
		state->duration = 1.0f;
}

void ActionStateSystem::ResetActionIntent(ActionIntent* intent)
{
	intent->attack = false;
	intent->dodge = false;
	intent->parry = false;
}
