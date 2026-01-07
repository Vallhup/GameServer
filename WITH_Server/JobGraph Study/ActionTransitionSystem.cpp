#include "pch.h"
#include "ActionTransitionSystem.h"

void ActionTransitionSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<ActionState>();
	auto& requests = ecs.GetStorage<ActionRequestTag>();

	std::vector<Entity> removeList;

	for (const auto& [entity, request] : requests)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (auto* state = states.GetComponent(entity))
		{
			ActionType next = ResolveNextAction(*state, request);
			if (next == state->type)
			{
				removeList.push_back(entity);
				continue;
			}

			ApplyTransition(state, next);
			removeList.push_back(entity);
		}
	}

	for (const auto& entity : removeList)
	{
		requests.RemoveComponent(entity);
	}
}

std::vector<std::type_index> ActionTransitionSystem::WriteComponents() const
{
	return { typeid(ActionState), typeid(ActionRequestTag) };
}

int ActionTransitionSystem::GetPriority(ActionType type)
{
	// Action 우선순위
	// Dead > Hit > Parry > Dodge > Attack > None

	switch (type) {
	case ActionType::Dead:	 return 100;
	case ActionType::Hit:	 return 90;
	case ActionType::Parry:  return 80;
	case ActionType::Dodge:	 return 70;
	case ActionType::Attack: return 60;
	case ActionType::None:	 return 0;
	default:                 return 0;	
	}
}

float ActionTransitionSystem::GetDuration(ActionType type)
{
	// TEMP : Action 별 Duration 값 설정 필요

	switch (type) {
	case ActionType::Dead:	 return 1;
	case ActionType::Hit:	 return 1;
	case ActionType::Parry:  return 1;
	case ActionType::Dodge:	 return 1;
	case ActionType::Attack: return 1;
	case ActionType::None:	 return 0;
	default:                 return 0;
	}
}

ActionType ActionTransitionSystem::ResolveNextAction(const ActionState& current, const ActionRequestTag& request)
{
	if (current.type == ActionType::Dead)
		return ActionType::Dead;

	if (GetPriority(request.type) > GetPriority(current.type))
		return request.type;

	if (current.type != ActionType::None && 
		current.elapsed >= current.duration)
	{
		return ActionType::None;
	}

	return current.type;
}

void ActionTransitionSystem::ApplyTransition(ActionState* state, ActionType next)
{
	state->type = next;
	state->elapsed = 0.0f;
	state->duration = GetDuration(next);
}
