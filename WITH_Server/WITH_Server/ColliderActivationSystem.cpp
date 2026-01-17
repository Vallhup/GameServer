#include "pch.h"
#include "ColliderActivationSystem.h"

void ColliderActivationSystem::Execute(const float dT)
{
	auto& colliders = ecs.GetStorage<Collider>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& attackStates = ecs.GetStorage<AttackState>();

	for (const auto& [entity, collider] : colliders)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* actionState = actionStates.GetComponent(entity);
		auto* attackState = attackStates.GetComponent(entity);
		if (!actionState || !attackState) continue;
		if (!collider.staticDatas) continue;

		const size_t n = collider.staticDatas->size();

#ifdef _DEBUG
		assert(collider.enabledMasks.size() == n);
		assert(collider.attackIds.size() == n);
#endif

		// 1. Attack 진입 시 Attack ID 증가
		//    (연속 공격은 어떻게 할지 고민 필요)
		const bool enteredAttack =
			(attackState->prevAction != ActionType::Attack) &&
			(actionState->type == ActionType::Attack);

		if (enteredAttack) ++attackState->attackId;
		attackState->prevAction = actionState->type;

		// 2. Attack / Parry 판정 윈도우 설정
		//    (elapsed 기반 / frame 기반 고민 필요)
		//    (현재는 elapsed 기반으로 임시 구현)
		const bool attackWindowOn =
			(actionState->type == ActionType::Attack) &&
			(actionState->elapsed >= 0.683f) &&
			(actionState->elapsed <= 0.975f);

		const bool parryWIndowOn = 
			(actionState->type == ActionType::Parry) &&
			(actionState->elapsed >= 0.752f) &&
			(actionState->elapsed <= 0.949f);

		const bool guardOn = (actionState->type == ActionType::Guard);

		// 3. Collider 활성화 설정
		for (size_t i = 0; i < n; ++i)
		{
			const uint8 typeMask = (*collider.staticDatas)[i].typeMask;

			const bool canHurt = HasType(typeMask, HitboxType::Hurt);
			const bool canHit = HasType(typeMask, HitboxType::Hit);

			const bool hitOn =
				canHit && (attackWindowOn || guardOn || parryWIndowOn);
			const bool offensiveHitOn = canHit && attackWindowOn;

			uint8 enabled{ 0 };
			if (canHurt) enabled |= static_cast<uint8>(HitboxType::Hurt);
			if (hitOn) enabled |= static_cast<uint8>(HitboxType::Hit);

			collider.enabledMasks[i] = enabled;
			collider.attackIds[i] = offensiveHitOn ? attackState->attackId : 0;
		}
	}
}