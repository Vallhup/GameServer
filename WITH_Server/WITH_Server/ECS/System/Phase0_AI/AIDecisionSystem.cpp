#include "pch.h"
#include "AIDecisionSystem.h"

#include <algorithm>
#include <DirectXMath.h>

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "RepComponent.h"

#include "AIPerceptionSystem.h"
#include "../../../GameplayContentCatalog.h"
#include "../AbilityProfileService.h"
#include "../Phase1/ApplyAICommandSystem.h"

#include "IAIReactionPolicy.h"
#include "IAISpecialActionPolicy.h"

using namespace GameplaySystemUtil;

namespace
{
	void TickAIActionRuntime(
		AIActionRuntimeComp& runtime,
		double dtSec) noexcept
	{
		const float dt = static_cast<float>(std::max(0.0, dtSec));
		const auto TickList =
			[dt](std::vector<float>& values) noexcept
			{
				for (float& value : values)
				{
					value = std::max(0.0f, value - dt);
				}
			};

		TickList(runtime.actionCooldownSec);
		TickList(runtime.groupCooldownSec);
		runtime.globalActionCooldownSec =
			std::max(0.0f, runtime.globalActionCooldownSec - dt);
		runtime.movementLockSec =
			std::max(0.0f, runtime.movementLockSec - dt);
	}

	bool IsAbilityAvailableForSelf(
		const AIContext& aiCtx,
		AbilityId abilityId) noexcept
	{
		if (abilityId == InvalidAbilityId || aiCtx.sysCtx == nullptr)
			return false;

		const SpawnTypeComp* spawnType =
			aiCtx.sysCtx->ecs.GetComponent<SpawnTypeComp>(aiCtx.self);
		if (spawnType == nullptr)
			return false;

		return AbilityProfileService::IsAbilityAvailable(
			spawnType->characterId,
			abilityId);
	}

	bool TryIssueReactionAbility(
		AIContext& aiCtx,
		const ReactionDecision& reactionDecision) noexcept
	{
		if (reactionDecision.reactAbilityId == InvalidAbilityId ||
			aiCtx.intent == nullptr ||
			aiCtx.abilityState == nullptr ||
			!aiCtx.abilityState->CanIssueAbility() ||
			!IsAbilityAvailableForSelf(aiCtx, reactionDecision.reactAbilityId))
		{
			return false;
		}

		aiCtx.intent->hasAbility = true;
		aiCtx.intent->abilityId = reactionDecision.reactAbilityId;
		aiCtx.intent->sequence++;
		return true;
	}

	void ApplyReactionRetarget(
		AIContext& aiCtx,
		const AIReactionEvent& event,
		const ReactionDecision& reactionDecision) noexcept
	{
		if (!reactionDecision.retargetAttacker ||
			aiCtx.blackboard == nullptr ||
			event.instigator.IsNull())
		{
			return;
		}

		aiCtx.blackboard->lastAttacker = event.instigator;
		aiCtx.blackboard->currentTarget = event.instigator;
		aiCtx.blackboard->forceRetarget = true;
	}

	void ApplyReactDurationOverride(
		AIContext& aiCtx,
		const ReactionDecision& reactionDecision) noexcept
	{
		if (aiCtx.decision == nullptr)
			return;

		aiCtx.decision->reactDurationOverrideActive =
			reactionDecision.hasReactDurationOverride;
		aiCtx.decision->reactDurationOverrideSec =
			reactionDecision.hasReactDurationOverride
			? reactionDecision.reactDurationSec
			: 0.0f;
	}
}

const StaticSystemMetaStorage<11, 1, 1> AIDecisionSystem::kMetaStorage =
MakeMetaStorage(
	SysTag<AIDecisionSystem>(),
	"AIDecisionSystem",
	std::array<AccessSpec, 11>
	{
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<AIPerceptionComp>()),
		ReadImmediate(ComponentRes<AITypeComp>()),
		WriteImmediate(ComponentRes<AIBlackboardComp>()),
		WriteImmediate(ComponentRes<AIDecisionComp>()),
		WriteImmediate(ComponentRes<AIIntentFrameComp>()),
		WriteImmediate(ComponentRes<AIReactionEventQueueComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<AIActionRuntimeComp>()),
		WriteImmediate(ComponentRes<AIMovementRuntimeComp>()),
	},
	std::array<SystemTag, 1>{ SysTag<ApplyAICommandSystem>() },
	std::array<SystemTag, 1>{ SysTag<AIPerceptionSystem>() }
);

void AIDecisionSystem::Execute(SystemContext& ctx)
{
	for (const auto& [entity, selfTr, abilityState, perception,
		blackboard, decision, intent, reaction, aiType, stats] :
		ctx.ecs.View<
		WorldTransformComp, AbilityStateComp, AIPerceptionComp,
		AIBlackboardComp, AIDecisionComp,
		AIIntentFrameComp, AIReactionEventQueueComp, AITypeComp, CombatStatStateComp>())
	{
		intent.ClearFrameTransient();

		AIActionRuntimeComp* actionRuntime =
			ctx.ecs.GetMutableComponent<AIActionRuntimeComp>(entity);
		AIMovementRuntimeComp* movementRuntime =
			ctx.ecs.GetMutableComponent<AIMovementRuntimeComp>(entity);
		if (actionRuntime != nullptr)
		{
			TickAIActionRuntime(*actionRuntime, ctx.dtSec);
		}

		decision.stateTime          += ctx.dtSec;
		decision.globalDecisionAcc  += ctx.dtSec;
		actionRuntime->idleActionCooldownAcc += ctx.dtSec;

		AIContext aiCtx;
		aiCtx.self            = entity;
		aiCtx.sysCtx          = &ctx;
		aiCtx.selfTr          = &selfTr;
		aiCtx.abilityState    = &abilityState;
		aiCtx.perception      = &perception;
		aiCtx.blackboard      = &blackboard;
		aiCtx.decision        = &decision;
		aiCtx.intent		  = &intent;
		aiCtx.reaction        = &reaction;
		aiCtx.stats           = &stats;
		aiCtx.actionRuntime   = actionRuntime;
		aiCtx.movementRuntime = movementRuntime;

		const AIFSMBundle* fsmBundle = _fsmRegistry.TryGetBundle(aiType.aiType);
		const AIBehaviorBundle* behaviorBundle = _fsmRegistry.TryGetBehavior(aiType.aiProfileId);

		if (fsmBundle != nullptr && behaviorBundle != nullptr)
		{
			aiCtx.movementPolicy		= behaviorBundle->movementPolicy.get();
			aiCtx.idleActionPolicy		= behaviorBundle->idleActionPolicy.get();
			aiCtx.combatActionPolicy	= behaviorBundle->combatActionPolicy.get();
			aiCtx.reactionPolicy		= behaviorBundle->reactionPolicy.get();
			aiCtx.specialActionPolicy	= behaviorBundle->specialActionPolicy.get();
			aiCtx.behaviorProfile		= behaviorBundle->profile;
			aiCtx.perceptionTuning		= &behaviorBundle->profile->perception;
			aiCtx.decisionTuning		= &behaviorBundle->profile->decision;

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

			ApplyReactionRetarget(ctx, *topEvent, reactionDecision);

			switch (reactionDecision.outcome) {
			case ReactionTacticalOutcome::EnterReact:
				ApplyReactDurationOverride(ctx, reactionDecision);
				ctx.decision->RequestTransition(AIStateType::React);
				if (ApplyPendingTransition(ctx, bundle))
					forceNextDecisionStep();
				break;

			case ReactionTacticalOutcome::ForceRetarget:
				break;

			case ReactionTacticalOutcome::IssueAbility:
				TryIssueReactionAbility(ctx, reactionDecision);
				break;

			case ReactionTacticalOutcome::EnterReactAndIssueAbility:
				TryIssueReactionAbility(ctx, reactionDecision);
				ApplyReactDurationOverride(ctx, reactionDecision);
				ctx.decision->RequestTransition(AIStateType::React);
				if (ApplyPendingTransition(ctx, bundle))
					forceNextDecisionStep();
				break;

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
