#include "pch.h"
#include "ResolveCombatHitSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveCombatHitSystem::kMeta =
	MakeSystemMeta<ResolveCombatHitSystem>("ResolveCombatHitSystem");

void ResolveCombatHitSystem::Execute(SystemContext& ctx)
{
	std::vector<Entity> attackers;
	for (auto [entity, activation] :
		ctx.ecs.View<CombatColliderActivationComp>())
	{
		if (activation.hasAttackWindow &&
			!HasBlockingPendingState(ctx.ecs, entity))
		{
			attackers.push_back(entity);
		}
	}

	std::sort(
		attackers.begin(),
		attackers.end(),
		[](Entity lhs, Entity rhs)
		{
			return lhs.id < rhs.id;
		});

	for (Entity attacker : attackers)
	{
		auto* attackerAction =
			ctx.ecs.GetMutableComponent<ActionStateComp>(attacker);
		auto* attackerTransform =
			ctx.ecs.GetMutableComponent<WorldTransformComp>(attacker);
		auto* attackerShape =
			ctx.ecs.GetMutableComponent<BodyCollisionShapeComp>(attacker);
		auto* attackerDedup =
			ctx.ecs.GetMutableComponent<CombatHitDedupStateComp>(attacker);
		if (attackerAction == nullptr || attackerTransform == nullptr ||
			attackerShape == nullptr)
		{
			continue;
		}

		if (attackerDedup != nullptr &&
			attackerDedup->boundActionInstanceId !=
				attackerAction->actionInstanceId)
		{
			attackerDedup->boundActionInstanceId =
				attackerAction->actionInstanceId;
			attackerDedup->resolvedVictims.clear();
		}

		const ActionDef* actionDef = FindActionDef(attackerAction->actionId);
		if (actionDef == nullptr || actionDef->duration <= 0.0f)
		{
			continue;
		}

		uint16_t windowIndex = 0;
		const float normalizedTime = ClampFloat(
			attackerAction->elapsedSec / actionDef->duration,
			0.0f,
			1.0f);
		const std::optional<AttackCombatEffectDef> attackEffect =
			FindCurrentAttackEffect(*actionDef, normalizedTime, windowIndex);
		if (!attackEffect.has_value())
		{
			continue;
		}

		for (auto [victim, victimTransform, victimShape, victimActivation] :
			ctx.ecs.View<
				WorldTransformComp,
				BodyCollisionShapeComp,
				CombatColliderActivationComp>())
		{
			if (victim == attacker ||
				HasBlockingPendingState(ctx.ecs, victim) ||
				IsSameFaction(ctx.ecs, attacker, victim) ||
				victimActivation.hasInvulnerabilityWindow)
			{
				continue;
			}

			if (attackerDedup != nullptr &&
				std::find(
					attackerDedup->resolvedVictims.begin(),
					attackerDedup->resolvedVictims.end(),
					victim) != attackerDedup->resolvedVictims.end())
			{
				continue;
			}

			const float dx = victimTransform.position.x -
				attackerTransform->position.x;
			const float dz = victimTransform.position.z -
				attackerTransform->position.z;
			const float hitRange =
				victimShape.bodyRadiusXZ + attackerShape->bodyRadiusXZ + 0.3f;
			if (LengthXZ(dx, dz) > hitRange)
			{
				continue;
			}

			PendingCombatResultComp* victimResult =
				ctx.ecs.GetMutableComponent<PendingCombatResultComp>(victim);
			if (victimResult == nullptr)
			{
				continue;
			}

			const CombatResolveResultType resultType =
				victimActivation.hasParryWindow
				? CombatResolveResultType::Parry
				: (victimActivation.hasGuardWindow
					? CombatResolveResultType::Guard
					: CombatResolveResultType::Hit);

			victimResult->receivedInteractions.push_back(
				PendingCombatInteractionRecord{
					attacker,
					attackerAction->actionId,
					attackerAction->actionInstanceId,
					windowIndex,
					0,
					0,
					resultType,
					*attackEffect,
					std::nullopt,
					std::nullopt,
					attackEffect->knockbackDistance,
					attackEffect->hitStopSec
				});
			victimResult->wasHitThisFrame |=
				resultType == CombatResolveResultType::Hit;
			victimResult->guardSucceededThisFrame |=
				resultType == CombatResolveResultType::Guard;
			victimResult->parrySucceededThisFrame |=
				resultType == CombatResolveResultType::Parry;
			if (resultType == CombatResolveResultType::Hit)
			{
				victimResult->reactionKind = CombatReactionKind::HitReaction;
			}

			if (attackerDedup != nullptr)
			{
				attackerDedup->resolvedVictims.push_back(victim);
			}
		}
	}
}

const SystemMeta& ResolveCombatHitSystem::Meta() const
{
	return kMeta;
}
