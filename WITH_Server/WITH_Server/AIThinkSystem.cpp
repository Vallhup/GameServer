#include "pch.h"
#include "AIThinkSystem.h"
#include "Framework.h"

#include "AI.h"
#include "Action.h"
#include "Tags.h"

void AIThinkSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	const auto& actionStates = ecs.GetStorage<ActionState>();
	auto& aiStates = ecs.GetStorage<AIState>();
	auto& aiThinkStates = ecs.GetStorage<AIThinkState>();

	for (const auto& [entity, aiState] : aiStates)
	{
		const auto* actionState = actionStates.GetComponent(entity);
		auto* aiThinkState = aiThinkStates.GetComponent(entity);
		if (!actionState || !aiThinkState) continue;
		
		if (actionState->action != ActionType::None && 
			actionState->action != ActionType::Hit) continue;

		aiThinkState->thinkAcc += dT;
		if (aiThinkState->thinkAcc >= aiThinkState->thinkInterval)
		{
			aiThinkState->thinkAcc -= aiThinkState->thinkInterval;

			AttackType next = Think(entity, &aiState);
			ActionRequestEvent ev
			{
				.entity		= entity,
				.actionType = ActionType::Attack,
				.attackType = next,
				.reason		= ActionRequestReason::FromAI
			};
			_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
		}
	}
}

AttackType AIThinkSystem::Think(Entity self, AIState* aiState)
{
	ECS& ecs = _runtime.GetECS();

	auto& transforms = ecs.GetStorage<Transform>();
	const auto* selfTransform = transforms.GetComponent(self);
	if (!selfTransform) return AttackType::None;

	if (aiState->target.id == Entity::InvalidId)
	{
		Entity target = FindTargetPlayer(self, *aiState, *selfTransform);
		if (target.id == Entity::InvalidId) return AttackType::None;

		aiState->target = target;
		aiState->patternsOnTarget = 0;
	}

	else if (aiState->patternsOnTarget > 3)
	{
		Entity target = FindFarthestPlayer(self, *selfTransform);
		if (target.id == Entity::InvalidId) return AttackType::None;

		aiState->target = target;
		aiState->patternsOnTarget = 0;
	}
	
	const auto* targetTransform = transforms.GetComponent(aiState->target);
	if (!targetTransform) return AttackType::None;

	const double targetDx = targetTransform->position.x - selfTransform->position.x;
	const double targetDz = targetTransform->position.z - selfTransform->position.z;
	const double targetDistance = targetDx * targetDx + targetDz * targetDz;

	// 메테오 조건
	const double nearDistance{ 5.0 * 5.0 };
	int nearCount{ 0 };

	auto& players = ecs.GetStorage<PlayerTag>();
	for (const auto& [entity, player] : players)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (entity == self) continue;

		const auto* pTransform = transforms.GetComponent(entity);
		if (!pTransform) continue;

		const double dx = pTransform->position.x - selfTransform->position.x;
		const double dz = pTransform->position.z - selfTransform->position.z;
		const double distance = dx * dx + dz * dz;

		if (distance <= nearDistance) 
			nearCount++;
	}

	AttackType out{ AttackType::None };

	const double midDistance{ 10.0 * 10.0 };
	const double farDistance{ 15.0 * 15.0 };

	if (nearCount >= 2 && rand() % 100 > 70)
		out = AttackType::Meteor;

	else if (targetDistance > farDistance)
	{
		if (rand() % 100 > 30)
			out = AttackType::JumpSlash;

		else
			out = AttackType::MultiSlash;
	}

	else if (targetDistance > midDistance)
		out = AttackType::DashSlash;

	else
	{
		if (rand() % 100 > 50)
			out = AttackType::Thrust;

		else
			out = AttackType::Slash;
	}

	if (out != AttackType::None)
		aiState->patternsOnTarget++;

	return out;
}

Entity AIThinkSystem::FindTargetPlayer(Entity self, const AIState& aiState, const Transform& transform)
{
	if (aiState.lastAttacker.id != Entity::InvalidId)
	{
		if (!_runtime.GetECS().GetStorage<DisconnectedTag>().HasComponent(aiState.lastAttacker))
			return aiState.lastAttacker;
	}

	return FindFarthestPlayer(self, transform);
}

Entity AIThinkSystem::FindFarthestPlayer(Entity self, const Transform& selfTrans)
{
	ECS& ecs = _runtime.GetECS();

	const auto& transforms = ecs.GetStorage<Transform>();
	auto& players = ecs.GetStorage<PlayerTag>();

	Entity best;
	double bestDistance{ 0.0f };

	for (const auto& [entity, player] : players)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		if (entity == self) continue;
		
		const auto* transform = transforms.GetComponent(entity);
		if (!transform) continue;

		const double dx = transform->position.x - selfTrans.position.x;
		const double dz = transform->position.z - selfTrans.position.z;
		const double distance = dx * dx + dz * dz;

		if (bestDistance < distance)
		{
			bestDistance = distance;
			best = entity;
		}
	}

	return best;
}