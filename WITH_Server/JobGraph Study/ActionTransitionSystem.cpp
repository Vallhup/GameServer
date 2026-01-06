#include "pch.h"
#include "ActionTransitionSystem.h"

void ActionTransitionSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<ActionState>();

	for (const auto& [entity, state] : states)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (state.type == ActionType::None) continue;
		if (state.elapsed < state.duration) continue;

		ActionTransition(&state);
	}
}

std::vector<std::type_index> ActionTransitionSystem::WriteComponents() const
{
	return { typeid(ActionState), typeid(LocomotionState) };
}

void ActionTransitionSystem::ActionTransition(ActionState* state)
{
	// TEMP : FSM 전이 로직 임시 구현
	state->type = ActionType::None;
	state->elapsed = 0.0f;
}
