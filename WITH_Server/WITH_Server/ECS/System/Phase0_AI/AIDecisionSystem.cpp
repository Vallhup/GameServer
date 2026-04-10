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

const StaticSystemMetaStorage<10, 1, 1> AIDecisionSystem::kMetaStorage =
MakeMetaStorage(
	SysTag<AIDecisionSystem>(),
	"AIDecisionSystem",
	std::array<AccessSpec, 10>
	{
		ReadSnapshot(ComponentRes<WorldTransformComp>()),
		ReadSnapshot(ComponentRes<ActionStateComp>()),
		ReadSnapshot(ComponentRes<AIPerceptionComp>()),
		ReadSnapshot(ComponentRes<AIPerceptionTuningComp>()),
		ReadSnapshot(ComponentRes<AIDecisionTuningComp>()),
		ReadSnapshot(ComponentRes<AITypeComp>()),
		WriteImmediate(ComponentRes<AIBlackboardComp>()),
		WriteImmediate(ComponentRes<AIDecisionComp>()),
		WriteImmediate(ComponentRes<AICommandFrameComp>()),
		WriteImmediate(ComponentRes<AIReactionComp>()),
	},
	std::array<SystemTag, 1>{ SysTag<ApplyAICommandSystem>() },
	std::array<SystemTag, 1>{ SysTag<AIPerceptionSystem>() }
);

void AIDecisionSystem::Execute(SystemContext& ctx)
{
	for (const auto& [entity, selfTr, actionState, perception, perceptionTuning,
		blackboard, decision, decisionTuning, command, reaction, aiType] :
		ctx.ecs.View<
		WorldTransformComp, ActionStateComp,
		AIPerceptionComp, AIPerceptionTuningComp,
		AIBlackboardComp, AIDecisionComp, AIDecisionTuningComp,
		AICommandFrameComp, AIReactionComp, AITypeComp>())
	{
		command.ClearFrameTransient();

		decision.stateTime          += ctx.dtSec;
		decision.globalDecisionAcc  += ctx.dtSec;
		decision.attackCooldownAcc  += ctx.dtSec;

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

		if (const AIFSMBundle* bundle = _fsmRegistry.TryGetBundle(aiType.aiType))
		{
			aiCtx.movementPolicy     = bundle->movementPolicy.get();
			aiCtx.combatActionPolicy = bundle->combatActionPolicy.get();
			aiCtx.reactionPolicy     = bundle->reactionPolicy.get();
			RunFSM(aiCtx, *bundle);
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
