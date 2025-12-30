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
			ActionType nextType = GetNextAction(*intent);
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

ActionType ActionStateSystem::GetNextAction(const ActionIntent& intent)
{
	// TEMP : 우선순위 변경 가능
	if (intent.parry) return ActionType::Parry;
	if (intent.dodge) return ActionType::Dodge;
	if(intent.attack) return ActionType::Attack;
	return ActionType::None;
}

bool ActionStateSystem::StartAction(ActionState* state, const ActionType& action)
{
	if (!state) return false;

	// TEMP : duration 값 수정 필요(Action별로 다르게)
	state->type = action;
	state->elapsed = 0.0f;
	state->duration = 1.0f;
}

void ActionStateSystem::ResetActionIntent(ActionIntent* intent)
{
	intent->attack = false;
	intent->dodge = false;
	intent->parry = false;
}
