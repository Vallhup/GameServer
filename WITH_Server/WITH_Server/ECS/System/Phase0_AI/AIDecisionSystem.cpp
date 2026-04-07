#include "pch.h"
#include "AIDecisionSystem.h"

#include <DirectXMath.h>

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "RepComponent.h"

using namespace GameplaySystemUtil;

const SystemMeta AIDecisionSystem::kMeta =
MakeSystemMeta<AIDecisionSystem>("AIDecisionSystem");

void AIDecisionSystem::Execute(SystemContext& ctx)
{
	for (const auto& [entity, selfTr, actionState, perception, perceptionTuning,
		blackboard, decision, decisionTuning, command, reaction, aiType] :
		ctx.ecs.View<WorldTransformComp, ActionStateComp, AIPerceptionComp,
		AIPerceptionTuningComp, AIBlackboardComp, AIDecisionComp, AIDecisionTuningComp,
		AICommandFrameComp, AIReactionComp, AITypeComp>())
	{
		command.ClearFrameTransient();

		decision.stateTime += ctx.dtSec;
		decision.globalDecisionAcc += ctx.dtSec;
		decision.attackCooldownAcc += ctx.dtSec;

		AIContext aiCtx;
		aiCtx.self = entity;
		aiCtx.sysCtx = &ctx;
		aiCtx.selfTr = &selfTr;
		aiCtx.actionState = &actionState;
		aiCtx.perception = &perception;
		aiCtx.perceptionTuning = &perceptionTuning;
		aiCtx.blackboard = &blackboard;
		aiCtx.decision = &decision;
		aiCtx.decisionTuning = &decisionTuning;
		aiCtx.command = &command;
		aiCtx.reaction = &reaction;

		if (const AIFSMBundle* bundle = _fsmRegistry.TryGetBundle(aiType.aiType))
		{
			aiCtx.movementPolicy = bundle->movementPolicy.get();
			RunFSM(aiCtx, *bundle);
		}

		reaction.Clear();
	}
}

void AIDecisionSystem::RunFSM(
	AIContext& ctx,
	const AIFSMBundle& bundle)
{
	const AIStateRegistry& states = bundle.stateRegistry;

	if (ctx.decision->enteredThisFrame)
	{
		if (const IAIState* initial = states.TryGetState(ctx.decision->curState))
			initial->Enter(ctx);

		ctx.decision->enteredThisFrame = false;
	}

	// 1. Reaction은 즉시 실행
	if (ctx.reaction->GotReactionEvent())
	{
		ctx.decision->RequestTransition(AIStateType::React);
		ApplyPendingTransition(ctx, bundle);
		return;
	}

	// 2. 매 프레임 motion/look 갱신
	if (const IAIState* current = states.TryGetState(ctx.decision->curState))
		current->FrameUpdate(ctx, ctx.sysCtx->dtSec);

	// 3. decision tick마다 판단 수행
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
	AIContext& ctx,
	const AIFSMBundle& bundle)
{
	if (!ctx.decision || !ctx.decision->transitionRequested)
		return;

	const AIStateType cur = ctx.decision->curState;
	const AIStateType next = ctx.decision->requestedState;

	if (cur == next)
	{
		ctx.decision->transitionRequested = false;
		return;
	}

	const AIStateRegistry& states = bundle.stateRegistry;

	if (const IAIState* curState = states.TryGetState(cur))
		curState->Exit(ctx);

	ctx.decision->prevState = cur;
	ctx.decision->curState = next;
	ctx.decision->stateTime = 0.0;
	ctx.decision->transitionRequested = false;

	if (const IAIState* nextState = states.TryGetState(next))
		nextState->Enter(ctx);
}