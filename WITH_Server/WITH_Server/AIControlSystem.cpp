#include "pch.h"
#include "AIControlSystem.h"

#include "AI.h"
#include "Movement.h"

#include "Event.h"

void AIControlSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, aiState, aiThinkState, aiIntent] :
		ecs.View<AIState, AIThinkState, AIIntent>())
	{
		switch (aiIntent.type) {
		case AIIntentType::None:
		case AIIntentType::Idle:
		{
			ApplyIdle(entity);
			break;
		}
		case AIIntentType::Chase:
		case AIIntentType::Retreate:
		case AIIntentType::Strafe:
		{
			ApplyMove(entity, aiIntent);
			break;
		}
		case AIIntentType::Attack:
		{
			if (!aiIntent.issued)
			{
				ApplyAttack(entity, aiState, aiThinkState, aiIntent);
				aiIntent.issued = true;
			}
			break;
		}
#ifdef _DEBUG
		default:
		{
			std::cout << "[AIControlSystem] Unknown AIIntentType: " <<
				static_cast<int>(aiIntent.type) << "\n";
		}
#endif
		}
	}
}

void AIControlSystem::ApplyIdle(Entity entity)
{
	ECS& ecs = _runtime.GetECS();

	auto* velocity = ecs.GetStorage<Velocity>().GetComponent(entity);
	auto* locomotion = ecs.GetStorage<LocomotionState>().GetComponent(entity);

	if (velocity)
	{
		velocity->dir = { 0.0f, 0.0f, 0.0f };
		velocity->isRun = false;
	}

	if (locomotion)
	{
		locomotion->isMoving = false;
		locomotion->isRun = false;
	}
}

void AIControlSystem::ApplyMove(Entity entity, const AIIntent& intent)
{
	ECS& ecs = _runtime.GetECS();

	auto* velocity = ecs.GetStorage<Velocity>().GetComponent(entity);
	auto* locomotion = ecs.GetStorage<LocomotionState>().GetComponent(entity);

	if (velocity)
	{
		velocity->dir = intent.moveDir;
		velocity->isRun = intent.isRun;
	}

	if (locomotion)
	{
		const float dirLengthSq =
			intent.moveDir.x * intent.moveDir.x + intent.moveDir.z * intent.moveDir.z;

		const bool hasMove = dirLengthSq > 1e-6;

		locomotion->isMoving = hasMove;
		locomotion->isRun = hasMove ? intent.isRun : false;
	}
}

void AIControlSystem::ApplyAttack(
	Entity entity, 
	AIState& aiState, 
	AIThinkState& aiThinkState,
	AIIntent& intent)
{
	// ∞¯∞› Ω√ ¿Ãµø ∏ÿ√„
	ApplyIdle(entity);

	if (intent.attackType == AttackType::None) return;

	ActionRequestEvent ev{};
	ev.entity		= entity;
	ev.actionType	= ActionType::Attack;
	ev.attackType	= intent.attackType;
	ev.reason		= ActionRequestReason::FromAI;

	_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);

	aiState.lastAttack = intent.attackType;
	aiState.patternsOnTarget++;
	aiThinkState.attackCooldownAcc = 0.0;
	intent.Clear();
}
