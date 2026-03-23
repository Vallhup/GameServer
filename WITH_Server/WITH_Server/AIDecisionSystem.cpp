#include "pch.h"
#include "AIDecisionSystem.h"

#include "AI.h"
#include "Action.h"

void AIDecisionSystem::Execute(const double dT)
{
	for (const auto& [entity, selfTr, actionState, perception, perceptionTuning,
		blackboard, decision, decisionTuning, intent, reaction, typeComp] :
		_runtime.GetECS().View<Transform, ActionState, AIPerceptionCache,
		AIPerceptionTuning, AIBlackboard, AIDecisionState, AIDecisionTuning,
		AIIntent, AIReactionCache, SpawnTypeComp>())
	{
		intent.ClearFrameTransient();

		decision.stateTime += dT;
		decision.globalDecisionAcc += dT;
		decision.attackCooldownAcc += dT;

		AIContext ctx;
		ctx.self = entity;
		ctx.runtime = &_runtime;
		ctx.selfTr = &selfTr;
		ctx.actionState = &actionState;
		ctx.perception = &perception;
		ctx.perceptionTuning = &perceptionTuning;
		ctx.blackboard = &blackboard;
		ctx.decision = &decision;
		ctx.decisionTuning = &decisionTuning;
		ctx.intent = &intent;
		ctx.reaction = &reaction;

		if (const AIFSMBundle* bundle = _fsmRegistry.TryGetBundle(typeComp.type))
		{
			ctx.movementPolicy = bundle->movementPolicy.get();
			RunFSM(ctx, *bundle, dT);
		}

		reaction.Clear();
	}
}

void AIDecisionSystem::RunFSM(
	AIContext& ctx, 
	const AIFSMBundle& bundle,
	const double dT)
{
	if (!ctx.IsValidContext()) return;

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
		current->FrameUpdate(ctx, dT);

	// 3. decision tick마다 판단 수행
	int steps{ 0 };
	while (ctx.decision->globalDecisionAcc >= ctx.decisionTuning->decisionInterval &&
		steps < kMaxDecisionStepPerFrame)
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
