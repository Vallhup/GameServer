#include "pch.h"
#include "AIDecisionSystem.h"

#include <DirectXMath.h>

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "RepComponent.h"

#include "AIPerceptionSystem.h"
#include "../Phase1/ApplyAICommandSystem.h"

#include "IAIReactionPolicy.h"

using namespace GameplaySystemUtil;

namespace
{
	void TickBossPatternRuntime(
		BossPatternRuntimeComp& runtime,
		double dtSec) noexcept
	{
		const float dt = static_cast<float>(dtSec);
		for (float& cooldown : runtime.patternCooldownSec)
		{
			cooldown = std::max(0.0f, cooldown - dt);
		}

		runtime.phaseTransitionLockSec =
			std::max(0.0f, runtime.phaseTransitionLockSec - dt);
		runtime.strafeTimeLeftSec =
			std::max(0.0f, runtime.strafeTimeLeftSec - dt);
	}

	bool TryFillDirectionToCurrentTarget(AIContext& ctx, float& outX, float& outZ)
	{
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
			ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(
				ctx.blackboard->currentTarget);
		if (targetTr == nullptr)
		{
			return false;
		}

		outX = targetTr->position.x - ctx.selfTr->position.x;
		outZ = targetTr->position.z - ctx.selfTr->position.z;
		NormalizeXZ(outX, outZ);
		return LengthXZ(outX, outZ) > kOverlapEpsilon;
	}

	bool TryIssuePendingBossTransitionAction(AIContext& ctx)
	{
		if (ctx.sysCtx == nullptr ||
			ctx.command == nullptr ||
			ctx.actionState == nullptr ||
			!ctx.actionState->CanIssueAction())
		{
			return false;
		}

		BossPatternRuntimeComp* runtime =
			ctx.sysCtx->ecs.GetMutableComponent<BossPatternRuntimeComp>(ctx.self);
		if (runtime == nullptr || !runtime->phaseTransitionActionPending)
		{
			return false;
		}

		float dirX = 0.0f;
		float dirZ = 0.0f;
		(void)TryFillDirectionToCurrentTarget(ctx, dirX, dirZ);

		ctx.command->ClearAll();
		ctx.command->hasLook = true;
		ctx.command->target = ctx.blackboard
			? ctx.blackboard->currentTarget
			: Entity::Null();
		ctx.command->hasAction = true;
		ctx.command->actionId = ActionId::BigDemonWarrior_BattleCry;
		ctx.command->actionDirX = dirX;
		ctx.command->actionDirZ = dirZ;
		ctx.command->sequence++;

		runtime->phaseTransitionActionPending = false;
		runtime->phaseTransitionLockSec =
			std::max(runtime->phaseTransitionLockSec, 3.6f);
		return true;
	}
}

const StaticSystemMetaStorage<12, 1, 1> AIDecisionSystem::kMetaStorage =
MakeMetaStorage(
	SysTag<AIDecisionSystem>(),
	"AIDecisionSystem",
	std::array<AccessSpec, 12>
	{
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<ActionStateComp>()),
		ReadImmediate(ComponentRes<AIPerceptionComp>()),
		ReadImmediate(ComponentRes<AIPerceptionTuningComp>()),
		ReadImmediate(ComponentRes<AIDecisionTuningComp>()),
		ReadImmediate(ComponentRes<AITypeComp>()),
		WriteImmediate(ComponentRes<AIBlackboardComp>()),
		WriteImmediate(ComponentRes<AIDecisionComp>()),
		WriteImmediate(ComponentRes<AICommandFrameComp>()),
		WriteImmediate(ComponentRes<AIReactionComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<BossPatternRuntimeComp>()),
	},
	std::array<SystemTag, 1>{ SysTag<ApplyAICommandSystem>() },
	std::array<SystemTag, 1>{ SysTag<AIPerceptionSystem>() }
);

void AIDecisionSystem::Execute(SystemContext& ctx)
{
	for (const auto& [entity, selfTr, actionState, perception, perceptionTuning,
		blackboard, decision, decisionTuning, command, reaction, aiType, stats] :
		ctx.ecs.View<
		WorldTransformComp, ActionStateComp,
		AIPerceptionComp, AIPerceptionTuningComp,
		AIBlackboardComp, AIDecisionComp, AIDecisionTuningComp,
		AICommandFrameComp, AIReactionComp, AITypeComp, CombatStatStateComp>())
	{
		command.ClearFrameTransient();

		decision.stateTime          += ctx.dtSec;
		decision.globalDecisionAcc  += ctx.dtSec;
		decision.attackCooldownAcc  += ctx.dtSec;
		blackboard.idleActionCooldownAcc += ctx.dtSec;

		AIContext aiCtx;
		aiCtx.self            = entity;
		aiCtx.sysCtx          = &ctx;
		aiCtx.selfTr          = &selfTr;
		aiCtx.actionState     = &actionState;
		aiCtx.perception      = &perception;
		aiCtx.perceptionTuning = &perceptionTuning;
		aiCtx.blackboard      = &blackboard;
		aiCtx.decision        = &decision;
		aiCtx.decisionTuning  = &decisionTuning;
		aiCtx.command         = &command;
		aiCtx.reaction        = &reaction;
		aiCtx.stats           = &stats;

		if (BossPatternRuntimeComp* bossRuntime =
			ctx.ecs.GetMutableComponent<BossPatternRuntimeComp>(entity))
		{
			TickBossPatternRuntime(*bossRuntime, ctx.dtSec);
		}

		const AIFSMBundle* fsmBundle = _fsmRegistry.TryGetBundle(aiType.aiType);
		const AIBehaviorBundle* behaviorBundle =
			_fsmRegistry.TryGetBehavior(
				aiType.aiType,
				static_cast<AITuningId>(aiType.aiTuningId));

		if (fsmBundle != nullptr && behaviorBundle != nullptr)
		{
			aiCtx.movementPolicy     = behaviorBundle->movementPolicy.get();
			aiCtx.combatActionPolicy = behaviorBundle->combatActionPolicy.get();
			aiCtx.reactionPolicy     = behaviorBundle->reactionPolicy.get();
			aiCtx.behaviorProfile    = behaviorBundle->profile;
			if (TryIssuePendingBossTransitionAction(aiCtx))
			{
				reaction.Clear();
				continue;
			}
			RunFSM(aiCtx, *fsmBundle);
		}

		reaction.Clear();
	}
}

void AIDecisionSystem::RunFSM(
	AIContext&        ctx,
	const AIFSMBundle& bundle)
{
	const AIStateRegistry& states = bundle.stateRegistry;

	if (ctx.decision->enteredThisFrame)
	{
		if (const IAIState* initial = states.TryGetState(ctx.decision->curState))
			initial->Enter(ctx);

		ctx.decision->enteredThisFrame = false;
	}

	// 1. 반응 이벤트 처리 — policy 가 결과를 결정
	if (ctx.reaction->HasAnyEvent())
	{
		const AIReactionEvent* topEvent = ctx.reaction->TopPriorityEvent();

		if (topEvent && ctx.reactionPolicy)
		{
			const ReactionDecision reactionDecision =
				ctx.reactionPolicy->Evaluate(*topEvent, ctx);

			// 타겟 재지정
			if (reactionDecision.retargetAttacker && !topEvent->instigator.IsNull())
			{
				ctx.blackboard->lastAttacker   = topEvent->instigator;
				ctx.blackboard->forceRetarget  = true;
			}

			// 전술 결과에 따른 상태 전환
			switch (reactionDecision.outcome) {
			case ReactionTacticalOutcome::EnterReact:
				ctx.decision->RequestTransition(AIStateType::React);
				ApplyPendingTransition(ctx, bundle);
				return;

			case ReactionTacticalOutcome::ForceRetarget:
				// 상태 전환 없이 blackboard 갱신만 — 이미 위에서 처리됨
				break;

			case ReactionTacticalOutcome::Ignore:
			default:
				break;
			}
		}
	}

	// 2. 매 프레임 motion/look 갱신
	if (const IAIState* current = states.TryGetState(ctx.decision->curState))
		current->FrameUpdate(ctx, ctx.sysCtx->dtSec);

	// 3. decision tick 마다 판단 수행
	int steps{ 0 };
	while (ctx.decision->globalDecisionAcc >= ctx.decisionTuning->decisionInterval &&
		steps < kMaxDecisionStepsPerFrame)
	{
		ctx.decision->globalDecisionAcc -= ctx.decisionTuning->decisionInterval;

		if (const IAIState* current = states.TryGetState(ctx.decision->curState))
			current->DecisionUpdate(ctx, ctx.decisionTuning->decisionInterval);

		ApplyPendingTransition(ctx, bundle);
		++steps;
	}
}

void AIDecisionSystem::ApplyPendingTransition(
	AIContext&         ctx,
	const AIFSMBundle& bundle)
{
	if (!ctx.decision || !ctx.decision->transitionRequested)
		return;

	const AIStateType cur  = ctx.decision->curState;
	const AIStateType next = ctx.decision->requestedState;

	if (cur == next)
	{
		ctx.decision->transitionRequested = false;
		return;
	}

	const AIStateRegistry& states = bundle.stateRegistry;

	if (const IAIState* curState = states.TryGetState(cur))
		curState->Exit(ctx);

	ctx.decision->prevState           = cur;
	ctx.decision->curState            = next;
	ctx.decision->stateTime           = 0.0;
	ctx.decision->transitionRequested = false;

	if (const IAIState* nextState = states.TryGetState(next))
		nextState->Enter(ctx);
}
