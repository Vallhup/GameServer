#include "pch.h"
#include "ActionTransitionSystem.h"
#include "Framework.h"
#include "Math.h"

ActionTransitionSystem::ActionTransitionSystem(ECS& e, int p)
	: System(e, p)
{
	for (auto& transitionRule : _transitionRules)
	{
		for (ActionType& type : transitionRule)
			type = Invalid;
	}

	LoadTransitionRules();
}

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
				return event.entity < e;
			});

		ActionType request{ ActionType::None };
		if (it != events.end() && it->entity == entity)
			request = it->type;
		
		if (auto* intent = intents.GetComponent(entity))
		{
			const bool guardHeld = intent->guard;

			if (actionState.type == ActionType::Guard)
			{
				if (!guardHeld)
					request = ActionType::None;

				else if (request == ActionType::None)
					request = ActionType::Guard;
			}

			if (guardHeld && actionState.type == ActionType::None &&
				request == ActionType::None)
			{
				request = ActionType::Guard;
			}
		
			ActionType next = ResolveNextAction(actionState, request, guardHeld);
			if (next != actionState.type)
				ApplyTransition(entity, &actionState, next);

			else if (next == ActionType::None)
				ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);
		}
	}

	events.clear();
}

ActionType ActionTransitionSystem::ResolveNextAction(const ActionState& current, 
	ActionType request, bool guardHeld)
{
	const ActionType curType = current.type;
	const auto& aM = ActionManager::Get();
	const auto& curPol = aM.GetPolicy(curType);

	if (curType == ActionType::Dead) 
		return ActionType::Dead;

	ActionType rule = GetRule(curType, request);
	if (rule != Invalid)
		return rule;

	const auto& reqPol = aM.GetPolicy(request);
	if (reqPol.priority > curPol.priority)
	{
		if (curPol.interruptMask & Bit(request))
			return request;
	}

	if (!curPol.isHoldAction && curType != ActionType::None &&
		current.elapsed >= curPol.duration)
	{
		if (guardHeld) return ActionType::Guard;
		return ActionType::None;
	}

	return curType;
}

void ActionTransitionSystem::ApplyTransition(Entity entity, ActionState* state, 
	ActionType next)
{
	const ActionType prev = state->type;
	const auto& aM = ActionManager::Get();

	const bool wasMoved = aM.GetPolicy(prev).isMoveAction;
	const bool isMove = aM.GetPolicy(next).isMoveAction;

	if (wasMoved && !isMove)
	{
		ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);
	}

	state->type = next;
	state->elapsed = 0.0f;
	state->duration = ActionManager::Get().GetPolicy(next).duration;

	if (isMove)
	{
		auto* move = ecs.GetStorage<ActionMoveTag>().AddComponent(entity);

		move->profile = aM.GetActionMoveProfile(state->type);
		move->segmentIndex = 0;
		move->movedInSegment = 0.0f;

		if (auto* trans = ecs.GetStorage<Transform>().GetComponent(entity))
		{
			XMStoreFloat3(&move->dir, TransformHelper::Forward(*trans));
			move->dirLocked = true;
		}

		//if (auto* vel = ecs.GetStorage<Velocity>().GetComponent(entity))
		//{
		//	// TEMP : 공격, 회피 방향 정책 수정 필요
		//	move->dir = vel->lastNonZeroDir;
		//	move->dirLocked = true;
		//}
	}
}

void ActionTransitionSystem::DedupActionRequest(std::vector<ActionRequestEvent>& events)
{
	if (events.empty()) return;

	std::sort(events.begin(), events.end(),
		[&](const ActionRequestEvent& a, const ActionRequestEvent& b)
		{
			const auto& aM = ActionManager::Get();
			const int32 aPriority = aM.GetPolicy(a.type).priority;
			const int32 bPriority = aM.GetPolicy(b.type).priority;

			if (a.entity != b.entity) return a.entity < b.entity;
			return aPriority > bPriority;
		});

	events.erase(std::unique(events.begin(), events.end(),
		[](const ActionRequestEvent& a, const ActionRequestEvent& b)
		{
			return a.entity == b.entity;
		}), events.end());
}

void ActionTransitionSystem::LoadTransitionRules()
{
	SetRule(ActionType::Guard, ActionType::Dead, ActionType::Dead);
	SetRule(ActionType::Guard, ActionType::Parry, ActionType::Parry);
	SetRule(ActionType::Guard, ActionType::Dodge, ActionType::Dodge);
	SetRule(ActionType::Guard, ActionType::Attack, ActionType::Attack);
	SetRule(ActionType::Guard, ActionType::Hit, ActionType::Guard);
	SetRule(ActionType::Guard, ActionType::Guard, ActionType::Guard);
	SetRule(ActionType::Guard, ActionType::None, ActionType::None);  
	   
	SetRule(ActionType::Stun, ActionType::Dead, ActionType::Dead);
	SetRule(ActionType::Stun, ActionType::Hit, ActionType::Hit);
}

void ActionTransitionSystem::SetRule(ActionType cur, ActionType req, ActionType next)
{
	_transitionRules[ToIndex(cur)][ToIndex(req)] = next;
}

ActionType ActionTransitionSystem::GetRule(ActionType cur, ActionType req) const
{
	return _transitionRules[ToIndex(cur)][ToIndex(req)];
}
