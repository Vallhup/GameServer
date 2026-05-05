#include "pch.h"
#include "BossAISpecialActionPolicy.h"

#include "AIBehaviorDef.h"
#include "IAIState.h"
#include "ECS/System/GameplaySystemUtil.h"

void BossAISpecialActionPolicy::TickRuntime(AIContext& ctx, const double dtSec) const
{
	if (ctx.bossPatternRuntime == nullptr) 
		return;

	BossPatternRuntimeComp* runtime = ctx.bossPatternRuntime;

	const float dt = static_cast<float>(dtSec);

	for (float& cooldown : runtime->patternCooldownSec)
	{
		cooldown = std::max(0.0f, cooldown - dt);
	}

	runtime->phaseTransitionLockSec =
		std::max(0.0f, runtime->phaseTransitionLockSec - dt);

	runtime->strafeTimeLeftSec =
		std::max(0.0f, runtime->strafeTimeLeftSec - dt);
}

bool BossAISpecialActionPolicy::TryIssuePreFSMAction(AIContext& ctx) const
{
	if (ctx.sysCtx == nullptr ||
		ctx.command == nullptr ||
		ctx.abilityState == nullptr ||
		!ctx.abilityState->CanIssueAbility() ||
		ctx.bossPatternRuntime == nullptr)
	{
		return false;
	}

	BossPatternRuntimeComp* runtime = ctx.bossPatternRuntime;
	if (!runtime->phaseTransitionActionPending)
	{
		return false;
	}

	// TEMP: 추후 데이터 테이블로 분리
	AbilityId transitionAbilityId = InvalidAbilityId;
	float transitionLockSec = 0.0f;
	uint8_t currentPhase = 1;
	if (const BossPhaseStateComp* phase =
		ctx.sysCtx->ecs.GetComponent<BossPhaseStateComp>(ctx.self))
	{
		currentPhase = phase->currentPhase;
	}

	if (ctx.behaviorProfile != nullptr)
	{
		for (const AIBossPhaseTransitionDef& transition :
			ctx.behaviorProfile->bossPattern.phaseTransitions)
		{
			if (transition.phase != currentPhase)
				continue;

			transitionAbilityId = transition.transitionAbilityId;
			transitionLockSec = transition.transitionLockSec;
			break;
		}
	}

	if (transitionAbilityId == InvalidAbilityId)
		return false;

	float dirX{ 0.0f };
	float dirZ{ 0.0f };
	TryFillDirectionToCurrentTarget(ctx, dirX, dirZ);

	ctx.command->ClearAll();
	ctx.command->hasLook = true;
	ctx.command->target =
		ctx.blackboard ?
		ctx.blackboard->currentTarget :
		Entity::Null();

	ctx.command->hasAbility = true;
	ctx.command->abilityId = transitionAbilityId;
	ctx.command->abilityDirX = dirX;
	ctx.command->abilityDirZ = dirZ;
	ctx.command->sequence++;

	runtime->phaseTransitionActionPending = false;
	runtime->phaseTransitionLockSec =
		std::max(runtime->phaseTransitionLockSec, transitionLockSec);

	return true;
}

bool BossAISpecialActionPolicy::TryFillDirectionToCurrentTarget(AIContext& ctx, float& outX, float& outZ) noexcept
{
	using namespace GameplaySystemUtil;

	outX = 0.0f;
	outZ = 0.0f;

	if (ctx.sysCtx == nullptr ||
		ctx.blackboard == nullptr ||
		ctx.blackboard->currentTarget.IsNull() ||
		ctx.selfTr == nullptr)
	{
		return false;
	}

	const WorldTransformComp* targetTr =
		ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(ctx.blackboard->currentTarget);

	if (targetTr == nullptr)
		return false;

	outX = targetTr->position.x - ctx.selfTr->position.x;
	outZ = targetTr->position.z - ctx.selfTr->position.z;

	NormalizeXZ(outX, outZ);
	return LengthXZ(outX, outZ) > kOverlapEpsilon;
}
