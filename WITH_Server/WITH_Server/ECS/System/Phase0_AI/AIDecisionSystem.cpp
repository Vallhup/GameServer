#include "pch.h"
#include "AIDecisionSystem.h"

#include <algorithm>
#include <DirectXMath.h>

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "RepComponent.h"

#include "AIPerceptionSystem.h"
#include "../../../GameplayContentCatalog.h"
#include "../Phase1/ApplyAICommandSystem.h"

#include "IAIReactionPolicy.h"
#include "IAISpecialActionPolicy.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<10, 1, 1> AIDecisionSystem::kMetaStorage =
MakeMetaStorage(
	SysTag<AIDecisionSystem>(),
	"AIDecisionSystem",
	std::array<AccessSpec, 10>
	{
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<AIPerceptionComp>()),
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
	for (const auto& [entity, selfTr, abilityState, perception,
		blackboard, decision, command, reaction, aiType, stats] :
		ctx.ecs.View<
		WorldTransformComp, AbilityStateComp, AIPerceptionComp,
		AIBlackboardComp, AIDecisionComp,
		AICommandFrameComp, AIReactionComp, AITypeComp, CombatStatStateComp>())
	{
		command.ClearFrameTransient();

		decision.stateTime          += ctx.dtSec;
		decision.globalDecisionAcc  += ctx.dtSec;
		if (!IsAbilityActive(abilityState))
		{
			decision.attackCooldownAcc += ctx.dtSec;
		}
		blackboard.idleActionCooldownAcc += ctx.dtSec;

		AIContext aiCtx;
		aiCtx.self            = entity;
		aiCtx.sysCtx          = &ctx;
		aiCtx.selfTr          = &selfTr;
		aiCtx.abilityState    = &abilityState;
		aiCtx.perception      = &perception;
		aiCtx.blackboard      = &blackboard;
		aiCtx.decision        = &decision;
		aiCtx.command         = &command;
		aiCtx.reaction        = &reaction;
		aiCtx.stats           = &stats;

		BossPatternRuntimeComp* bossRuntime = ctx.ecs.GetMutableComponent<BossPatternRuntimeComp>(entity);

		const AIFSMBundle* fsmBundle = _fsmRegistry.TryGetBundle(aiType.aiType);
		const AIBehaviorBundle* behaviorBundle = _fsmRegistry.TryGetBehavior(aiType.aiProfileId);

		if (fsmBundle != nullptr && behaviorBundle != nullptr)
		{
			aiCtx.movementPolicy		= behaviorBundle->movementPolicy.get();
			aiCtx.combatActionPolicy	= behaviorBundle->combatActionPolicy.get();
			aiCtx.reactionPolicy		= behaviorBundle->reactionPolicy.get();
			aiCtx.specialActionPolicy	= behaviorBundle->specialActionPolicy.get();
			aiCtx.behaviorProfile		= behaviorBundle->profile;
			aiCtx.perceptionTuning		= &behaviorBundle->profile->perception;
			aiCtx.decisionTuning		= &behaviorBundle->profile->decision;
			aiCtx.bossPatternRuntime	= bossRuntime;

			if (aiCtx.specialActionPolicy != nullptr)
			{
				aiCtx.specialActionPolicy->TickRuntime(aiCtx, ctx.dtSec);

				if (aiCtx.specialActionPolicy->TryIssuePreFSMAction(aiCtx))
				{
					reaction.Clear();
					continue;
				}
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
	const double decisionInterval =
		(ctx.decisionTuning != nullptr)
		? ctx.decisionTuning->decisionInterval
		: 0.0;

	const auto forceNextDecisionStep =
		[&ctx, decisionInterval]()
		{
			if (ctx.decision == nullptr || decisionInterval <= 0.0)
				return;

			ctx.decision->globalDecisionAcc =
				std::max(ctx.decision->globalDecisionAcc, decisionInterval);
		};

	if (ctx.decision->enteredThisFrame)
	{
		if (const IAIState* initial = states.TryGetState(ctx.decision->curState))
			initial->Enter(ctx);

		ctx.decision->enteredThisFrame = false;
	}

	if (ctx.reaction->HasAnyEvent())
	{
		const AIReactionEvent* topEvent = ctx.reaction->TopPriorityEvent();

		if (topEvent && ctx.reactionPolicy)
		{
			const ReactionDecision reactionDecision =
				ctx.reactionPolicy->Evaluate(*topEvent, ctx);

			{
				ctx.blackboard->lastAttacker   = topEvent->instigator;
				ctx.blackboard->forceRetarget  = true;
			}

			switch (reactionDecision.outcome) {
			case ReactionTacticalOutcome::EnterReact:
				ctx.decision->RequestTransition(AIStateType::React);
				if (ApplyPendingTransition(ctx, bundle))
					forceNextDecisionStep();
				break;

			case ReactionTacticalOutcome::ForceRetarget:

			case ReactionTacticalOutcome::Ignore:
			default:
				break;
			}
		}
	}

	int steps{ 0 };
	while (decisionInterval > 0.0 &&
		ctx.decision->globalDecisionAcc >= decisionInterval &&
		steps < kMaxDecisionStepsPerFrame)
	{
		ctx.decision->globalDecisionAcc -= decisionInterval;

		if (const IAIState* current = states.TryGetState(ctx.decision->curState))
			current->DecisionUpdate(ctx, decisionInterval);

		if (ApplyPendingTransition(ctx, bundle))
			forceNextDecisionStep();
		++steps;
	}

	if (const IAIState* current = states.TryGetState(ctx.decision->curState))
		current->FrameUpdate(ctx, ctx.sysCtx->dtSec);
}

bool AIDecisionSystem::ApplyPendingTransition(
	AIContext&         ctx,
	const AIFSMBundle& bundle)
{
	if (!ctx.decision || !ctx.decision->transitionRequested)
		return false;

	const AIStateType cur  = ctx.decision->curState;
	const AIStateType next = ctx.decision->requestedState;

	if (cur == next)
	{
		ctx.decision->transitionRequested = false;
		return false;
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

	return true;
}
