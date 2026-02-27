#include "pch.h"
#include "ColliderActivationSystem.h"
#include "Tags.h"
#include "RepComponent.h"

void ColliderActivationSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	const auto& actionStates = ecs.GetStorage<ActionState>();
	auto& colliders = ecs.GetStorage<CombatCollider>();
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

		const auto* typeComp = ecs.GetStorage<SpawnTypeComp>().GetComponent(entity);
		if (!typeComp) continue;

		// 1. Attack 진입 시 Attack ID 증가
		//    (연속 공격은 어떻게 할지 고민 필요)
		const bool enteredAttack =
			(attackState->prevAction != ActionType::Attack) &&
			(actionState->action == ActionType::Attack);

		if (enteredAttack)
		{
			++attackState->attackId;
			attackState->hitCount = 0;
		}
		attackState->prevAction = actionState->action;

		// 2. Attack / Parry 판정 윈도우 설정
		//    (하드코딩되어있는 판정 윈도우 값 데이터 분리 필요)
		const auto& pol = ActionManager::Get().GetPolicy(actionState->action, typeComp->type, actionState->attack);
		const float baseDuration = pol.duration;

		const float atkStartN = 0.683f / baseDuration;
		const float atkEndN = 0.975f / baseDuration;

		const float parryStartN = 0.752f;
		const float parryEndN = 0.949f;

		const float progress = static_cast<float>(actionState->progress);

		const bool attackWindowOn =
			(actionState->action == ActionType::Attack) &&
			(progress >= atkStartN && progress <= atkEndN);

		const bool parryWIndowOn =
			(actionState->action == ActionType::Parry) &&
			(progress >= parryStartN && progress <= parryEndN);

		const bool guardOn = (actionState->action == ActionType::Guard);
		const bool dodgeOn = (actionState->action == ActionType::Dodge);

		// 3. Collider 활성화 설정
		for (size_t i = 0; i < n; ++i)
		{
			if (dodgeOn)
			{
				collider.enabledMasks[i] = 0;
				collider.attackIds[i] = 0;
				continue;
			}

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