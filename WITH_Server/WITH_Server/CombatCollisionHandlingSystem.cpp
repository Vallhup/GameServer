#include "pch.h"
#include "CombatCollisionHandlingSystem.h"
#include "Math.h"

void CombatCollisionHandlingSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	auto events = _runtime.Events().Queue<CombatCollisionEvent>().ConsumeView();
	for (const auto& event : events)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(event.attacker)) continue;
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(event.victim)) continue;
		
		if (!ConsumeHitOnce(event)) continue;
		switch (event.type) {
		case CollisionType::Clash:
			HandleClash(event);
			break;

		case CollisionType::Strike:
			HandleStrike(event);
			break;
		}
	}

	_runtime.Events().Queue<CombatCollisionEvent>().Clear();
}

bool CombatCollisionHandlingSystem::ConsumeHitOnce(const CombatCollisionEvent& event)
{
	auto* atkState = 
		_runtime.GetECS().GetStorage<AttackState>().GetComponent(event.attacker);
	if (!atkState) return false;
	if (atkState->attackId == event.attackId &&
		atkState->HasHit(event.victim)) return false;

	atkState->MarkHit(event.victim);
	return true;
}

void CombatCollisionHandlingSystem::HandleClash(const CombatCollisionEvent& event)
{
	if (const ActionState* actionState =
		_runtime.GetECS().GetStorage<ActionState>().GetComponent(event.victim))
	{
		const bool parryWindowOn =
			(actionState->type == ActionType::Parry) &&
			(actionState->elapsed >= 0.752f) &&
			(actionState->elapsed <= 0.949f);

		if (parryWindowOn)
			HandleParry(event.attacker, event.victim, event.attackId);

		else
			HandleGuard(event.attacker, event.victim, event.attackId);
	}
}

void CombatCollisionHandlingSystem::HandleStrike(const CombatCollisionEvent& event)
{
	ECS& ecs = _runtime.GetECS();

	const ActionState* actionState =
		ecs.GetStorage<ActionState>().GetComponent(event.victim);

	const Transform* aTrans =
		ecs.GetStorage<Transform>().GetComponent(event.attacker);

	const Transform* vTrans =
		ecs.GetStorage<Transform>().GetComponent(event.victim);

	if (!actionState || !aTrans || !vTrans) return;

	const bool inFront90 = 
		TransformHelper::IsInFront90_XZ(*aTrans, *vTrans);

	const bool parryWindowOn =
		(actionState->type == ActionType::Parry) &&
		(actionState->elapsed >= 0.001f) && //0.752f
		(actionState->elapsed <= 0.949f);

	const bool guardOn = (actionState->type == ActionType::Guard);

	if (guardOn && inFront90)
		HandleGuard(event.attacker, event.victim, event.attackId);

	else if (parryWindowOn && inFront90)
		HandleParry(event.attacker, event.victim, event.attackId);

	else
		HandleHit(event.attacker, event.victim, event.attackId);
}

void CombatCollisionHandlingSystem::HandleParry(Entity attacker, 
	Entity victim, uint32 attackId)
{
	if (ParryBuf* parryBuf =
		_runtime.GetECS().GetStorage<ParryBuf>().GetComponent(victim))
	{
		parryBuf->remaining = 1;
		ActionRequestEvent ev
		{
			.entity = attacker,
			.actionType = ActionType::Stun,
			.attackType = AttackType::None,
			.reason = ActionRequestReason::FromCombat
		};
		_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
	}
}

void CombatCollisionHandlingSystem::HandleGuard(Entity attacker, 
	Entity victim, uint32 attackId)
{
	// TODO : 추후 방어력 추가해서 데미지 감소
}

void CombatCollisionHandlingSystem::HandleHit(Entity attacker, 
	Entity victim, uint32 attackId)
{
	ECS& ecs = _runtime.GetECS();

	auto* health = ecs.GetStorage<Health>().GetComponent(victim);
	if (!health) return;

	auto computeDamage =
		[&](Entity attacker) -> int
		{
			int damage{ 0 };
			if (const auto* ad = ecs.GetStorage<AttackData>().GetComponent(attacker))
				damage = ad->damage;

			if (auto* pb = ecs.GetStorage<ParryBuf>().GetComponent(attacker))
			{
				if (pb->remaining > 0)
				{
					// TEMP : 데미지 공식 정해야됨
					//        (현재는 임시로 additionalDamage만큼 배율로 때리고있음)
					const double mul = 1.0f + pb->additionalDamage;
					pb->remaining -= 1;

					if (pb->remaining <= 0)
						pb->remaining = 0;

					damage = std::lround(damage * mul);
				}
			}

			return damage;
		};

	const int damage = computeDamage(attacker);
	health->current -= damage;

	const bool isDeath = health->current <= 0;

	if (isDeath)
 		health->current = 0;

	ActionRequestEvent ev
	{
		.entity = victim,
		.actionType = isDeath ? ActionType::Dead : ActionType::Hit,
		.attackType = AttackType::None,
		.reason = ActionRequestReason::FromCombat
	};
	_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
}
