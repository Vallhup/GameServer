#include "pch.h"
#include "ActionTransitionSystem.h"
#include "Framework.h"
#include "Math.h"

ActionTransitionSystem::ActionTransitionSystem(WorldRuntime& rt, int p)
	: System(rt, p)
{
	for (auto& transitionRule : _transitionRules)
	{
		for (ActionType& type : transitionRule)
			type = Invalid;
	}

	LoadTransitionRules();
}

void ActionTransitionSystem::Execute(const double dT)
{
	auto evView = _runtime.Events().Queue<ActionRequestEvent>().ConsumeView();
	auto events = DedupActionRequest(evView);

	auto& ecs = _runtime.GetECS();

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

		ActionRequestEvent request
		{ entity, ActionType::None, AttackType::None, ActionRequestReason::None };
		bool hasReq{ false };
		if (it != events.end() && it->entity == entity)
		{
			request = *it;
			hasReq = true;
		}

		const bool isForced =
			hasReq &&
			(request.reason == ActionRequestReason::FromCombat);
		
		bool guardHeld{ false };
		if (auto* intent = intents.GetComponent(entity))
		{
			guardHeld = intent->guard;

			if(!isForced)
			{
				if (actionState.type == ActionType::Guard)
				{
					if (!guardHeld)
						request.actionType = ActionType::None;

					else if (request.actionType == ActionType::None)
						request.actionType = ActionType::Guard;
				}

				if (guardHeld && actionState.type == ActionType::None &&
					request.actionType == ActionType::None)
				{
					request.actionType = ActionType::Guard;
				}
			}
		}

		ActionType next =
			ResolveNextAction(actionState, request, guardHeld, isForced);
		if (next != actionState.type)
			ApplyTransition(entity, &actionState, next);

		else if (next == ActionType::None)
			ecs.GetStorage<ActionMoveTag>().RemoveComponent(entity);
	}
}

ActionType ActionTransitionSystem::ResolveNextAction(const ActionState& current, 
	ActionRequestEvent request, bool guardHeld, bool isForced)
{
	if (isForced) return request.actionType;

	const ActionType curType = current.type;
	const auto& aM = ActionManager::Get();
	const auto& curPol = aM.GetPolicy(curType);

	if (curType == ActionType::Dead) 
		return ActionType::Dead;

	ActionType rule = GetRule(curType, request.actionType);
	if (rule != Invalid)
		return rule;

	const auto& reqPol = aM.GetPolicy(request.actionType);
	if (reqPol.priority > curPol.priority)
	{
		if (curPol.interruptMask & Bit(request.actionType))
			return request.actionType;
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
	ECS& ecs = _runtime.GetECS();

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
			// TEMP : 나중에 데이터로 분리
			move->dirLocked = true;
		}
	}
}

std::span<ActionRequestEvent> ActionTransitionSystem::DedupActionRequest(std::span<ActionRequestEvent> events)
{
	if (events.empty()) return events;

	std::sort(events.begin(), events.end(),
		[&](const ActionRequestEvent& a, const ActionRequestEvent& b)
		{
			const auto& aM = ActionManager::Get();
			const int32 aPriority = aM.GetPolicy(a.actionType).priority;
			const int32 bPriority = aM.GetPolicy(b.actionType).priority;

			if (a.entity != b.entity) return a.entity < b.entity;
			return aPriority > bPriority;
		});

	size_t w{ 0 };
	for (size_t i = 0; i < events.size(); ++i)
	{
		if (w == 0 || events[i].entity != events[w - 1].entity)
			events[w++] = events[i];
	}

	return events.first(w);
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
