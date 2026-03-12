#include "pch.h"
#include "CombatCollisionHandlingSystem.h"
#include "Math.h"
#include "Tags.h"
#include "RepComponent.h"
#include "Framework.h"

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
			(actionState->action == ActionType::Parry) &&
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
		(actionState->action == ActionType::Parry) &&
		(actionState->elapsed >= 0.001f) && //0.752f
		(actionState->elapsed <= 0.949f);

	const bool guardOn = (actionState->action == ActionType::Guard);

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
	ECS& ecs = _runtime.GetECS();

	auto* vitals = ecs.GetStorage<Vital>().GetComponent(victim);
	if (!vitals) return;

	auto computeDamage =
		[&](Entity attacker, Entity victim) -> int
		{
			int damage{ 0 };
			if (const auto* aAttr = ecs.GetStorage<FinalAttribute>().GetComponent(attacker))
				damage = aAttr->power;

			if (auto* pBuf = ecs.GetStorage<ParryBuf>().GetComponent(attacker))
			{
				if (pBuf->remaining > 0)
				{
					const double mul = 1.0f + pBuf->additionalDamage;
					pBuf->remaining -= 1;

					if (pBuf->remaining <= 0)
						pBuf->remaining = 0;

					damage = std::lround(damage * mul);
				}
			}

			if (const auto* vAttr = ecs.GetStorage<FinalAttribute>().GetComponent(victim))
			{
				damage = damage * (100 / (100 + vAttr->defense));
			}

			return damage;
		};

	int damage = computeDamage(attacker, victim);
	vitals->curHp -= damage;

	const bool isDeath = vitals->curHp <= 0;

	if (isDeath)
		vitals->curHp = 0;

	const auto* player = ecs.GetStorage<PlayerTag>().GetComponent(victim);
	if (player && damage > 0)
	{
		_runtime.MarkDirty(victim, WorldDirtyType::Stat);
	}

	ActionRequestEvent ev
	{
		.entity = victim,
		.actionType = isDeath ? ActionType::Dead : ActionType::Hit,
		.attackType = AttackType::None,
		.reason = ActionRequestReason::FromCombat
	};
	_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
}

void CombatCollisionHandlingSystem::HandleHit(Entity attacker, 
	Entity victim, uint32 attackId)
{
	ECS& ecs = _runtime.GetECS();

	auto* vitals = ecs.GetStorage<Vital>().GetComponent(victim);
	if (!vitals) return;

	auto computeDamage =
		[&](Entity attacker) -> int
		{
			int damage{ 0 };
			if (const auto* attribute = ecs.GetStorage<FinalAttribute>().GetComponent(attacker))
				damage = attribute->power;

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
	vitals->curHp -= damage;

	const bool isDeath = vitals->curHp <= 0;

	if (isDeath)
		vitals->curHp = 0;

	const auto* player = ecs.GetStorage<PlayerTag>().GetComponent(victim);
	if (player && damage > 0)
	{
		_runtime.MarkDirty(victim, WorldDirtyType::Stat);
	}

	ActionRequestEvent ev
	{
		.entity = victim,
		.actionType = isDeath ? ActionType::Dead : ActionType::Hit,
		.attackType = AttackType::None,
		.reason = ActionRequestReason::FromCombat
	};
	_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
}
