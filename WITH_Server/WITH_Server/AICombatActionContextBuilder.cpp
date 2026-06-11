#include "pch.h"
#include "AICombatActionContextBuilder.h"

#include "System.h"

bool AICombatActionContextBuilder::CanBuild(const AIContext& ctx) noexcept
{
	if (ctx.behaviorProfile == nullptr ||
		ctx.behaviorProfile->combatActionDefs.empty() ||
		ctx.perception == nullptr ||
		!ctx.perception->hasTarget)
	{
		return false;
	}

	return
		ctx.actionRuntime == nullptr ||
		ctx.actionRuntime->globalActionCooldownSec <= 0.0f;
}

AICombatActionSelectionContext AICombatActionContextBuilder::Build(
	const AIContext& ctx) noexcept
{
	const uint8_t phase = ResolvePhase(ctx);
	const AIMovementProfileDef* movement =
		SelectMovementProfile(ctx, phase);
	const double distanceToTarget = ctx.perception != nullptr
		? ctx.perception->distanceToTarget
		: std::numeric_limits<double>::max();

	return AICombatActionSelectionContext
	{
		.aiCtx = ctx,
		.runtime = ctx.actionRuntime,
		.actions = ctx.behaviorProfile->combatActionDefs,
		.movement = movement,
		.currentBucket =
			ResolveDistanceBucket(movement, distanceToTarget),
		.distanceToTarget = distanceToTarget,
		.phase = phase,
		.selfHpRatio = ResolveHpRatio(ctx.stats),
		.targetHpRatio = ResolveTargetHpRatio(ctx),
		.lastUsedAbilityId = ctx.actionRuntime != nullptr
			? ctx.actionRuntime->lastUsedAbilityId
			: InvalidAbilityId,
		.actionSequence = ctx.actionRuntime != nullptr
			? ctx.actionRuntime->actionSequence
			: 0u,
		.basicActionCountSinceEffect = ctx.actionRuntime != nullptr
			? ctx.actionRuntime->basicActionCountSinceEffect
			: 0u
	};
}

uint8_t AICombatActionContextBuilder::ResolvePhase(
	const AIContext& ctx) noexcept
{
	if (ctx.sysCtx == nullptr)
		return 1;

	const AIPhaseRuntimeComp* phaseComp =
		ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self);
	return phaseComp != nullptr
		? std::max<uint8_t>(1, phaseComp->currentPhase)
		: 1;
}

const AIMovementProfileDef*
AICombatActionContextBuilder::SelectMovementProfile(
	const AIContext& ctx,
	uint8_t phase) noexcept
{
	for (const AIMovementProfileDef& profile :
		ctx.behaviorProfile->movementProfiles)
	{
		if (phase >= profile.phaseMin && phase <= profile.phaseMax)
			return &profile;
	}

	return ctx.behaviorProfile->movementProfiles.empty()
		? nullptr
		: &ctx.behaviorProfile->movementProfiles.front();
}

AIActionDistanceBucket
AICombatActionContextBuilder::ResolveDistanceBucket(
	const AIMovementProfileDef* movement,
	double distance) noexcept
{
	if (movement == nullptr)
		return AIActionDistanceBucket::Any;

	if (distance <= movement->veryCloseDistance)
		return AIActionDistanceBucket::VeryClose;

	if (distance <= movement->closeDistance)
		return AIActionDistanceBucket::Close;

	if (distance <= movement->midDistance)
		return AIActionDistanceBucket::Mid;

	return AIActionDistanceBucket::Far;
}

float AICombatActionContextBuilder::ResolveHpRatio(
	const CombatStatStateComp* stats) noexcept
{
	if (stats == nullptr || stats->maxHp <= 0)
		return 1.0f;

	const float hpRatio =
		static_cast<float>(stats->currentHp) /
		static_cast<float>(stats->maxHp);
	return std::clamp(hpRatio, 0.0f, 1.0f);
}

float AICombatActionContextBuilder::ResolveTargetHpRatio(
	const AIContext& ctx) noexcept
{
	Entity target = Entity::Null();
	if (ctx.perception != nullptr &&
		!ctx.perception->selectedTarget.IsNull())
	{
		target = ctx.perception->selectedTarget;
	}
	else if (ctx.blackboard != nullptr &&
		!ctx.blackboard->currentTarget.IsNull())
	{
		target = ctx.blackboard->currentTarget;
	}

	if (target.IsNull() || ctx.sysCtx == nullptr)
		return 1.0f;

	return ResolveHpRatio(
		ctx.sysCtx->ecs.GetComponent<CombatStatStateComp>(target));
}
