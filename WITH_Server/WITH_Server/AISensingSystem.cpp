#include "pch.h"
#include "AISensingSystem.h"

#include "AI.h"
#include "Tags.h"
#include "Transform.h"

void AISensingSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, aiState, sense, tuning, selfTrans] :
		ecs.View<AIState, AISenseState, AICombatTuning, Transform>
		(Exclude<DisconnectedTag>()))
	{
		sense.Clear();

		Entity resolvedTarget = ResolveTarget(entity, aiState, selfTrans);
		aiState.target = resolvedTarget;

		if (resolvedTarget.IsNull())
		{
			sense.targetLost = true;
			continue;
		}

		FillSense(entity, aiState, selfTrans, tuning, sense);
	}
}

bool AISensingSystem::IsValidTarget(Entity target) const
{
	if (target.IsNull())
		return false;

	ECS& ecs = _runtime.GetECS();

	return
		!ecs.GetStorage<DisconnectedTag>().HasComponent(target) &&
		ecs.GetStorage<PlayerTag>().HasComponent(target) &&
		ecs.GetStorage<Transform>().HasComponent(target);
}

Entity AISensingSystem::ResolveTarget(
	Entity self, 
	AIState& aiState, 
	const Transform& selfTrans)
{
	if (IsValidTarget(aiState.lastAttacker))
		return aiState.lastAttacker;

	if (IsValidTarget(aiState.target))
		return aiState.target;

	return FindClosestPlayer(self, selfTrans);
}

Entity AISensingSystem::FindClosestPlayer(Entity self, const Transform& selfTrans)
{
	ECS& ecs = _runtime.GetECS();

	Entity best;
	double bestDistSq{ std::numeric_limits<double>::max() };

	for (const auto& [entity, playerTag, transform] :
		ecs.View<PlayerTag, Transform>(Exclude<DisconnectedTag>()))
	{
		if (entity == self) continue;

		const double distSq = TransformHelper::DistanceSq(selfTrans, transform);
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			best = entity;
		}
	}

	return best;
}

void AISensingSystem::FillSense(
	Entity self, 
	const AIState& aiState, 
	const Transform& selfTrans, 
	const AICombatTuning& tuning, 
	AISenseState& outSense)
{
	ECS& ecs = _runtime.GetECS();

	const Entity target = aiState.target;
	const auto* targetTrans = ecs.GetStorage<Transform>().GetComponent(target);
	if (!targetTrans)
	{
		outSense.targetLost = true;
		return;
	}

	const double loseTargetDist = tuning.loseTargetDistance;
	const double preferredMin = tuning.preferredMinDistance;
	const double preferredMax = tuning.preferredMaxDistance;
	const double attackDist = tuning.attackDistance;

	const double distSq = TransformHelper::DistanceSq(selfTrans, *targetTrans);
	const double dist = std::sqrt(distSq);

	outSense.hasTarget = true;
	outSense.targetLost = dist > loseTargetDist;
	outSense.target = target;

	outSense.distSqToTarget = distSq;
	outSense.distToTarget = dist;

	outSense.toTargetDir = MakeFlatDir(selfTrans, *targetTrans);
	outSense.awayFromTargetDir = MakeFlatDir(*targetTrans, selfTrans);

	outSense.targetTooClose = dist < preferredMin;
	outSense.targetTooFar = dist > preferredMax;
	outSense.targetInPreferredRange =
		!outSense.targetTooClose && !outSense.targetTooFar;

	outSense.targetInAttackRange = dist <= attackDist;

	const double nearbyRangeSq = tuning.meteorCheckDistance * tuning.meteorCheckDistance;
	outSense.nearbyPlayerCount = CountPlayersInRange(self, selfTrans, nearbyRangeSq);

	if (outSense.targetLost)
	{
		outSense.hasTarget = false;
	}
}

int AISensingSystem::CountPlayersInRange(
	Entity self, 
	const Transform& center, 
	float rangeSq) const
{
	ECS& ecs = _runtime.GetECS();

	int count{ 0 };
	for (const auto& [entity, playerTag, transform] :
		ecs.View<PlayerTag, Transform>(Exclude<DisconnectedTag>()))
	{
		if (entity == self) continue;

		const double distSq = TransformHelper::DistanceSq(center, transform);
		if (distSq <= rangeSq) ++count;
	}

	return count;
}

XMFLOAT3 AISensingSystem::MakeFlatDir(const Transform& from, const Transform& to)
{
	XMFLOAT3 dir
	{
		to.position.x - from.position.x,
		0.0f,
		to.position.z - from.position.z
	};

	XMVECTOR v = XMLoadFloat3(&dir);
	TransformHelper::SafeNormalize3(v, v);
	XMStoreFloat3(&dir, v);

	return dir;
}


