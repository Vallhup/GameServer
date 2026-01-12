#include "pch.h"
#include "ActionTransitionSystem.h"

void ActionTransitionSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<ActionState>();
	auto& requests = ecs.GetStorage<ActionRequestTag>();
	auto& velocities = ecs.GetStorage<Velocity>();

	std::vector<Entity> removeList;
	removeList.reserve(requests.Size());

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

			//if (next == ActionType::Dodge)
			//{
			//	auto* dodge = ecs.GetStorage<DodgeTag>().AddComponent(entity);

			//	auto* velocity = velocities.GetComponent(entity);
			//	if (!velocity) continue;

			//	dodge->dir = velocity->dir; 

			//	// TEMP : Dodge 거리 값 설정 필요
			//	dodge->distance = 3.5f;        
			//}

			ApplyTransition(state, next);
			removeList.push_back(entity);
		}
	}

	for (const auto& entity : removeList)
	{
		requests.RemoveComponent(entity);
	}
}

std::vector<std::type_index> ActionTransitionSystem::ReadComponents() const
{
	return { typeid(Velocity) };
}

std::vector<std::type_index> ActionTransitionSystem::WriteComponents() const
{
	return { typeid(ActionState), typeid(ActionRequestTag), typeid(ActionMoveTag) };
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

bool ActionTransitionSystem::CanBeInterrupted(const ActionState& current, const ActionRequestTag& request)
{
	switch (current.type) {
	case ActionType::Attack:
	case ActionType::Dodge:
	case ActionType::Parry:
	case ActionType::Stun:
		return request.type == ActionType::Hit ||
			request.type == ActionType::Dead;

	case ActionType::None:
		return true;

	default:
		return false;
	}
}

ActionType ActionTransitionSystem::ResolveNextAction(const ActionState& current, const ActionRequestTag& request)
{
	if (current.type == ActionType::Dead)
		return ActionType::Dead;

	if (current.type == ActionType::Guard)
	{
		switch (request.type) {
		case ActionType::Dead:
			return ActionType::Dead;

		case ActionType::Parry:
		case ActionType::Dodge:
		case ActionType::Attack:
			// Guard 해제 + 요청 Action으로 전이
			return request.type;

		case ActionType::Hit:
			// Guard 중에는 Hit 무시
			return ActionType::Guard;

		case ActionType::None:
			// Guard 입력 해제 -> None으로 전환
			return ActionType::None;

		default:
			return ActionType::Guard;
		}

	}

	if (current.type == ActionType::Stun)
	{
		switch (request.type) {
		case ActionType::Dead:
			return ActionType::Dead;

		case ActionType::Hit:
			return ActionType::Hit;

		default:
			return ActionType::Stun;
		}
	}

	if (GetPriority(request.type) > GetPriority(current.type))
	{
		if (CanBeInterrupted(current, request))
			return request.type;

		return current.type;
	}
		

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

	if (state->type == ActionType::Guard)
		state->duration = std::numeric_limits<float>::infinity();

	else
		state->duration = GetDuration(next);
}
