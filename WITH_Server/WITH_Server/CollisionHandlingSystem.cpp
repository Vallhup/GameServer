#include "pch.h"
#include "CollisionHandlingSystem.h"

void CollisionHandlingSystem::Execute(const float dT)
{
	auto& events = ecs.collisionEvents;

	for (const auto& event : events)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(event.attacker)) continue;
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(event.victim)) continue;

		switch (event.type) {
		case CollisionType::Clash:
			HandleClash(event);
			break;

		case CollisionType::Strike:
			HandleStrike(event);
			break;
		}
	}

	events.clear();
}

void CollisionHandlingSystem::HandleClash(const CollisionEvent& event)
{
	if (const ActionState* actionState =
		ecs.GetStorage<ActionState>().GetComponent(event.victim))
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

void CollisionHandlingSystem::HandleStrike(const CollisionEvent& event)
{
	if (const ActionState* actionState =
		ecs.GetStorage<ActionState>().GetComponent(event.victim))
	{
		const bool parryWindowOn =
			(actionState->type == ActionType::Parry) &&
			(actionState->elapsed >= 0.752f) &&
			(actionState->elapsed <= 0.949f);

		const bool guardOn = (actionState->type == ActionType::Guard);

		if (guardOn)
			HandleGuard(event.attacker, event.victim, event.attackId);

		else if (parryWindowOn)
			HandleParry(event.attacker, event.victim, event.attackId);

		else
			HandleHit(event.attacker, event.victim, event.attackId);
	}
}

void CollisionHandlingSystem::HandleParry(Entity attacker, Entity victim, 
	uint32 attackId)
{
	if (ParryBuf* parryBuf =
		ecs.GetStorage<ParryBuf>().GetComponent(victim))
	{
		parryBuf->remaining = 1;
		ecs.actionRequestEvents.emplace_back(attacker, ActionType::Stun);
	}
}

void CollisionHandlingSystem::HandleGuard(Entity attacker, Entity victim, 
	uint32 attackId)
{
	// TODO : 추후 방어력 추가해서 데미지 감소
}

void CollisionHandlingSystem::HandleHit(Entity attacker, Entity victim, 
	uint32 attackId)
{
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
					const float mul = 1.0f + pb->additionalDamage;
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

	const bool isDeath = health->current < 0;

	if (isDeath)
		health->current = 0;

	ecs.actionRequestEvents.emplace_back(victim,
		isDeath ? ActionType::Dead : ActionType::Hit);
}
