#include "pch.h"
#include "ActionTransitionSystem.h"

void ActionTransitionSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<ActionState>();
	auto& requests = ecs.GetStorage<ActionRequestTag>();

	std::vector<Entity> removeList;
	removeList.reserve(requests.Size());

	for (const auto& [entity, request] : requests)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (auto* state = states.GetComponent(entity))
		{
			ActionType next = ResolveNextAction(*state, request);
			if (next == ActionType::None)
			{
				ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);
			}

			if (next == state->type)
			{
				removeList.push_back(entity);
				continue;
			}

			ApplyTransition(entity, state, next);
			removeList.push_back(entity);
		}
	}

	for (const auto& entity : removeList)
	{
		requests.RemoveComponent(entity);
	}
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
	switch (type) {
	case ActionType::Dead:	 return 150.0f / 30.2013f;
	case ActionType::Hit:	 return 50.0f / 30.6122f;
	case ActionType::Parry:  return 54.0f / 30.566f;
	case ActionType::Dodge:	 return 50.0f / 30.6122f;
	case ActionType::Attack: return 40.0f / 30.7692f;
	case ActionType::Stun:	 return 96.0f / 30.3158f;
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

void ActionTransitionSystem::ApplyTransition(Entity entity, 
	ActionState* state, ActionType next)
{
	state->type = next;
	state->elapsed = 0.0f;

	if (state->type == ActionType::Guard)
		state->duration = std::numeric_limits<float>::infinity();

	else
		state->duration = GetDuration(next);

	if (state->type == ActionType::Attack || 
		state->type == ActionType::Dodge)
	{
		auto* move = ecs.GetStorage<ActionMoveTag>().AddComponent(entity);

		move->profile = ActionManager::Get().GetActionMoveProfile(state->type);
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
