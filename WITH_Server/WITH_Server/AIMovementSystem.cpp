#include "pch.h"
#include "AIMovementSystem.h"
#include "AI.h"
#include "Action.h"
#include "Movement.h"

void AIMovementSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	const auto& actionStates = ecs.GetStorage<ActionState>();
	const auto& transforms = ecs.GetStorage<Transform>();
	const auto& aiStates = ecs.GetStorage<AIState>();
	auto& velocities = ecs.GetStorage<Velocity>();

	for (const auto& [entity, aiState] : aiStates)
	{
		const auto* trans = transforms.GetComponent(entity);
		const auto* action = actionStates.GetComponent(entity);
		auto* vel = velocities.GetComponent(entity);
		if (!trans || !action || !vel) continue;

		if (action->action != ActionType::None)
		{
			vel->dir = { 0, 0, 0 };
			continue;
		}

		Entity target = aiState.target;
		if (target.IsNull())
		{
			vel->dir = { 0, 0, 0 };
			continue;
		}

		const auto* targetTr = transforms.GetComponent(target);
		if (!targetTr)
		{
			vel->dir = { 0, 0, 0 };
			continue;
		}

		XMVECTOR selfPos = XMLoadFloat3(&trans->position);
		XMVECTOR targetPos = XMLoadFloat3(&targetTr->position);
		XMVECTOR toTarget = XMVectorSubtract(targetPos, selfPos);
		toTarget = XMVectorSetY(toTarget, 0.0f);

		XMVECTOR dir;
		if (!TransformHelper::SafeNormalize3(toTarget, dir))
		{
			vel->dir = { 0, 0, 0 };
			continue;
		}

		XMStoreFloat3(&vel->dir, dir);
	}
}