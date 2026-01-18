#include "pch.h"
#include "ActionTransitionSystem.h"

void ActionTransitionSystem::Execute(const float dT)
{
	auto& events = ecs.actionRequestEvents;
	DedupActionRequest(events);

	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& intents = ecs.GetStorage<ActionIntent>();

	for (const auto& [entity, actionState] : actionStates)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto it = std::lower_bound(events.begin(), events.end(), entity,
			[](const ActionRequestEvent& event, Entity e)
			{
				return event.entity.id < e.id;
			});

		ActionType request{ ActionType::None };
		if (it != events.end() && it->entity == entity) 
			request = it->type;

		if (auto* intent = intents.GetComponent(entity))
		{
			if (actionState.type == ActionType::Guard && 
				intent->guard == false)
				request = ActionType::None;

			else if (request == ActionType::None &&
				actionState.type == ActionType::None && 
				intent->guard == true)
				request = ActionType::Guard;
		}

		ActionType next = ResolveNextAction(actionState, request);
		if (next != actionState.type)
			ApplyTransition(entity, &actionState, next);

		else if (next == ActionType::None)
			ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);
	}

	events.clear();
}

int ActionTransitionSystem::GetPriority(ActionType type)
{
	// Action 우선순위
	// Dead > Hit > Stun > Parry > Dodge > Attack > None

	switch (type) {
	case ActionType::Dead:	 return 100;
	case ActionType::Hit:	 return 90;
	case ActionType::Stun:	 return 85;
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
	default:                 return 0.0f;
	}
}

bool ActionTransitionSystem::CanBeInterrupted(const ActionState& current, 
	ActionType request)
{
	switch (current.type) {
	case ActionType::Attack:
	case ActionType::Dodge:
	case ActionType::Parry:
	case ActionType::Stun:
		return request == ActionType::Stun ||
			request == ActionType::Hit ||
			request == ActionType::Dead;

	case ActionType::Guard:
	case ActionType::None:
		return true;

	default:
		return false;
	}
}

ActionType ActionTransitionSystem::ResolveNextAction(const ActionState& current, 
	ActionType request)
{
	if (current.type == ActionType::Dead)
		return ActionType::Dead;

	if (current.type == ActionType::Guard)
	{
		switch (request) {
		case ActionType::Dead: 
			return ActionType::Dead;

		case ActionType::Parry:
		case ActionType::Dodge:
		case ActionType::Attack:
			// Guard 해제 + 요청 Action으로 전이
			return request;

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
		switch (request) {
		case ActionType::Dead: return ActionType::Dead;
		case ActionType::Hit: return ActionType::Hit;
		default: return ActionType::Stun;
		}
	}

	if (GetPriority(request) > GetPriority(current.type))
	{
		if (CanBeInterrupted(current, request))
			return request;

		return current.type;
	}
		

	if (current.type != ActionType::None && current.elapsed >= current.duration)
	{
		return ActionType::None;
	}

	return current.type;
}

void ActionTransitionSystem::ApplyTransition(Entity entity, ActionState* state, 
	ActionType next)
{
	const ActionType prev = state->type;

	const bool wasMoved =
		(prev == ActionType::Attack) || (prev == ActionType::Dodge);

	const bool isMove =
		(next == ActionType::Attack) || (next == ActionType::Dodge);

	if (wasMoved && !isMove)
	{
		ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);
	}

	state->type = next;
	state->elapsed = 0.0f;

	if (state->type == ActionType::Guard)
		state->duration = std::numeric_limits<float>::infinity();

	else
		state->duration = GetDuration(next);

	if (isMove)
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

void ActionTransitionSystem::DedupActionRequest(std::vector<ActionRequestEvent>& events)
{
	if (events.empty()) return;

	std::sort(events.begin(), events.end(),
		[&](const ActionRequestEvent& a, const ActionRequestEvent& b)
		{
			if (a.entity != b.entity) return a.entity.id < b.entity.id;
			return GetPriority(a.type) > GetPriority(b.type);
		});

	events.erase(std::unique(events.begin(), events.end(),
		[](const ActionRequestEvent& a, const ActionRequestEvent& b)
		{
			return a.entity == b.entity;
		}), events.end());
}
