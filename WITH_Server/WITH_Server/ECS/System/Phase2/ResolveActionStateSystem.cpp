#include "pch.h"
#include "ResolveActionStateSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

#include <limits>

using namespace GameplaySystemUtil;

const SystemMeta ResolveActionStateSystem::kMeta =
	MakeSystemMeta<ResolveActionStateSystem>("ResolveActionStateSystem");

void ResolveActionStateSystem::Execute(SystemContext& ctx)
{
	const ActionProfileService profileService{};

	for (auto [
		entity,
		actionState,
		locomotionState,
		transform,
		input,
		advance,
		spawnType] :
		ctx.ecs.View<
			ActionStateComp,
			LocomotionStateComp,
			WorldTransformComp,
			ActorInputComp,
			ActionTimelineAdvanceComp,
			SpawnTypeComp>())
	{
		ClearActionTimelineAdvance(advance);

		if (TryHandleBlockingState(
			ctx,
			entity,
			actionState,
			input,
			advance))
		{
			continue;
		}

		const CharacterId characterId = spawnType.characterId;
		const CombatStatStateComp* stats =
			ctx.ecs.GetComponent<CombatStatStateComp>(entity);
		const AIPerceptionComp* perception =
			ctx.ecs.GetComponent<AIPerceptionComp>(entity);
		const ActionInterruptQueueComp* interruptQueue =
			ctx.ecs.GetComponent<ActionInterruptQueueComp>(entity);

		const ActionDef* currentActionDef =
			IsActionActive(actionState)
			? FindActionDef(actionState.actionId)
			: nullptr;

		if (IsActionActive(actionState) && currentActionDef == nullptr)
		{
			actionState = {};
			ClearActionInput(input);
			continue;
		}

		TransitionDecision decision{};
		const bool hadInterruptEvents =
			interruptQueue != nullptr && !interruptQueue->events.empty();
		if (hadInterruptEvents &&
			TryResolveInterruptTransition(
				profileService,
				characterId,
				actionState,
				currentActionDef,
				interruptQueue,
				decision))
		{
			ApplyTransition(actionState, decision);
			ConsumeOnRequestResourceCosts(ctx, entity, decision);
			if (IsActionActive(actionState))
			{
				PrepareStartedActionAdvance(actionState, advance);
			}
			if (ActionInterruptQueueComp* mutableInterruptQueue =
				ctx.ecs.GetMutableComponent<ActionInterruptQueueComp>(entity))
			{
				mutableInterruptQueue->events.clear();
			}
			ClearActionInput(input);
			continue;
		}

		if (hadInterruptEvents)
		{
			if (ActionInterruptQueueComp* mutableInterruptQueue =
				ctx.ecs.GetMutableComponent<ActionInterruptQueueComp>(entity))
			{
				mutableInterruptQueue->events.clear();
			}
		}

		if (currentActionDef != nullptr)
		{
			if (TryResolveCancelTransition(
				profileService,
				characterId,
				actionState,
				*currentActionDef,
				locomotionState,
				transform,
				input,
				stats,
				perception,
				decision))
			{
				ApplyTransition(actionState, decision);
				ConsumeOnRequestResourceCosts(ctx, entity, decision);
				SetActionDirectionOnStart(
					actionState,
					decision,
					locomotionState,
					transform);
				if (IsActionActive(actionState))
				{
					PrepareStartedActionAdvance(actionState, advance);
				}
				ClearActionInput(input);
				continue;
			}

			(void)TryResolveEndPolicyTransition(
				actionState,
				*currentActionDef,
				input,
				advance,
				ctx.dtSec,
				decision);
			if (decision.transition)
			{
				ApplyTransition(actionState, decision);
				ConsumeOnRequestResourceCosts(ctx, entity, decision);
				if (IsActionActive(actionState))
				{
					PrepareStartedActionAdvance(actionState, advance);
				}
			}

			ClearActionInput(input);
			continue;
		}

		if (TryResolveIdleRequestTransition(
			profileService,
			characterId,
			locomotionState,
			transform,
			input,
			stats,
			perception,
			decision))
		{
			ApplyTransition(actionState, decision);
			ConsumeOnRequestResourceCosts(ctx, entity, decision);
			SetActionDirectionOnStart(
				actionState,
				decision,
				locomotionState,
				transform);
			if (IsActionActive(actionState))
			{
				PrepareStartedActionAdvance(actionState, advance);
			}
		}

		ClearActionInput(input);
	}
}

const SystemMeta& ResolveActionStateSystem::Meta() const
{
	return kMeta;
}

bool ResolveActionStateSystem::TryHandleBlockingState(
	SystemContext& ctx,
	Entity entity,
	ActionStateComp& actionState,
	ActorInputComp& input,
	ActionTimelineAdvanceComp& advance)
{
	if (!HasBlockingPendingState(ctx.ecs, entity))
	{
		return false;
	}

	ClearActionTimelineAdvance(advance);
	actionState.actionId = ActionId::None;
	actionState.elapsedSec = 0.0f;
	ResetActionDirection(actionState);
	ClearActionInput(input);
	return true;
}

bool ResolveActionStateSystem::TryResolveInterruptTransition(
	const ActionProfileService& profileService,
	const CharacterId characterId,
	const ActionStateComp& actionState,
	const ActionDef* currentActionDef,
	const ActionInterruptQueueComp* interruptQueue,
	TransitionDecision& outDecision)
{
	if (interruptQueue == nullptr || interruptQueue->events.empty())
	{
		return false;
	}

	int bestPriority = std::numeric_limits<int>::min();
	ActionId bestActionId = ActionId::None;

	if (currentActionDef != nullptr)
	{
		const float progress =
			(currentActionDef->duration > 0.0f)
			? ClampFloat(actionState.elapsedSec / currentActionDef->duration, 0.0f, 1.0f)
			: 1.0f;

		for (const ActionInterruptEvent& event : interruptQueue->events)
		{
			for (const ActionInterruptRule& rule :
				currentActionDef->transitionRule.interruptRules)
			{
				if (rule.causeType != event.causeType)
				{
					continue;
				}

				if (rule.windowPolicy == ActionWindowPolicy::Range)
				{
					const float windowStart = rule.windowStartNormalized.value_or(0.0f);
					const float windowEnd = rule.windowEndNormalized.value_or(1.0f);
					if (progress < windowStart || progress > windowEnd)
					{
						continue;
					}
				}

				if (rule.priority > bestPriority)
				{
					if (!profileService.IsActionAvailable(characterId, rule.toActionId))
					{
						continue;
					}

					bestPriority = rule.priority;
					bestActionId = rule.toActionId;
				}
			}
		}
	}
	else
	{
		const ActionFallbackReactionProfileDef* fallbackProfile =
			profileService.FindFallbackReactionProfile(characterId);
		if (fallbackProfile == nullptr)
		{
			return false;
		}

		for (const ActionInterruptEvent& event : interruptQueue->events)
		{
			for (const ActionFallbackReactionEntryDef& entry :
				fallbackProfile->entries)
			{
				if (entry.causeType != event.causeType)
				{
					continue;
				}

				if (entry.priority > bestPriority)
				{
					if (!profileService.IsActionAvailable(characterId, entry.toActionId))
					{
						continue;
					}

					bestPriority = entry.priority;
					bestActionId = entry.toActionId;
				}
			}
		}
	}

	if (bestActionId == ActionId::None)
	{
		return false;
	}

	outDecision.transition = true;
	outDecision.nextActionId = bestActionId;
	outDecision.preserveDirection = false;
	return true;
}

bool ResolveActionStateSystem::TryResolveCancelTransition(
	const ActionProfileService& profileService,
	const CharacterId characterId,
	const ActionStateComp& actionState,
	const ActionDef& currentActionDef,
	const LocomotionStateComp& locomotionState,
	const WorldTransformComp& transform,
	const ActorInputComp& input,
	const CombatStatStateComp* stats,
	const AIPerceptionComp* perception,
	TransitionDecision& outDecision)
{
	const std::vector<RequestCandidate> candidates =
		BuildRequestCandidates(
			profileService,
			characterId,
			input,
			false);
	if (candidates.empty())
	{
		return false;
	}

	int bestPriority = std::numeric_limits<int>::min();
	RequestCandidate bestCandidate{};
	bool found = false;

	for (const RequestCandidate& candidate : candidates)
	{
		if (candidate.actionId == ActionId::None ||
			!profileService.IsActionAvailable(characterId, candidate.actionId) ||
			!IsActionRequestAllowed(candidate.actionId, stats, perception))
		{
			continue;
		}

		for (const ActionCancelRule& cancelRule :
			currentActionDef.transitionRule.cancelRules)
		{
			if (cancelRule.toActionId != candidate.actionId ||
				!IsCancelRuleActive(cancelRule, currentActionDef, actionState))
			{
				continue;
			}

			if (!found || cancelRule.priority > bestPriority)
			{
				bestPriority = cancelRule.priority;
				bestCandidate = candidate;
				found = true;
			}
		}
	}

	if (!found)
	{
		return false;
	}

	outDecision.transition = true;
	outDecision.nextActionId = bestCandidate.actionId;
	outDecision.consumeOnRequestCosts = true;
	outDecision.preserveDirection = true;
	outDecision.directionX = bestCandidate.directionX;
	outDecision.directionZ = bestCandidate.directionZ;
	(void)locomotionState;
	(void)transform;
	return true;
}

bool ResolveActionStateSystem::TryResolveIdleRequestTransition(
	const ActionProfileService& profileService,
	const CharacterId characterId,
	const LocomotionStateComp& locomotionState,
	const WorldTransformComp& transform,
	const ActorInputComp& input,
	const CombatStatStateComp* stats,
	const AIPerceptionComp* perception,
	TransitionDecision& outDecision)
{
	const std::vector<RequestCandidate> candidates =
		BuildRequestCandidates(
			profileService,
			characterId,
			input,
			true);
	for (const RequestCandidate& candidate : candidates)
	{
		if (candidate.actionId == ActionId::None ||
			!profileService.IsActionAvailable(characterId, candidate.actionId) ||
			!IsActionRequestAllowed(candidate.actionId, stats, perception))
		{
			continue;
		}

		outDecision.transition = true;
		outDecision.nextActionId = candidate.actionId;
		outDecision.consumeOnRequestCosts = true;
		outDecision.preserveDirection = true;
		outDecision.directionX = candidate.directionX;
		outDecision.directionZ = candidate.directionZ;
		(void)locomotionState;
		(void)transform;
		return true;
	}

	return false;
}

bool ResolveActionStateSystem::TryResolveEndPolicyTransition(
	const ActionStateComp& actionState,
	const ActionDef& actionDef,
	const ActorInputComp& input,
	ActionTimelineAdvanceComp& advance,
	double deltaTimeSec,
	TransitionDecision& outDecision)
{
	if (IsHoldReleased(actionDef, input))
	{
		outDecision.transition = true;
		outDecision.nextActionId = actionDef.endPolicy.defaultNextActionId;
		outDecision.preserveDirection = false;
		return true;
	}

	PrepareTimelineAdvance(actionState, actionDef, advance, deltaTimeSec);

	if (actionDef.normalizedPolicy == ActionNormalizedPolicy::Holdable &&
		actionDef.endPolicy.endType == ActionEndType::HoldRelease)
	{
		return false;
	}

	if (advance.currElapsedSec < actionDef.duration)
	{
		return false;
	}

	if (actionDef.kind == ActionKind::Dead)
	{
		return false;
	}

	outDecision.transition = true;
	outDecision.nextActionId = actionDef.endPolicy.defaultNextActionId;
	outDecision.preserveDirection = false;
	return true;
}

void ResolveActionStateSystem::ApplyTransition(
	ActionStateComp& actionState,
	const TransitionDecision& decision)
{
	if (!decision.transition)
	{
		return;
	}

	++actionState.actionInstanceId;
	actionState.actionId = decision.nextActionId;
	actionState.elapsedSec = 0.0f;
	ResetActionDirection(actionState);
}

void ResolveActionStateSystem::ConsumeOnRequestResourceCosts(
	SystemContext& ctx,
	Entity entity,
	const TransitionDecision& decision)
{
	if (!decision.transition || !decision.consumeOnRequestCosts)
	{
		return;
	}

	const ActionDef* actionDef = FindActionDef(decision.nextActionId);
	if (actionDef == nullptr)
	{
		return;
	}

	CombatStatStateComp* stats =
		ctx.ecs.GetMutableComponent<CombatStatStateComp>(entity);
	if (stats == nullptr)
	{
		return;
	}

	bool statDirty = false;
	for (const ActionResourceCostDef& cost : actionDef->resourceCosts)
	{
		if (cost.consumeTiming != ActionResourceConsumeTiming::OnRequest)
		{
			continue;
		}

		const int32_t amount = static_cast<int32_t>(std::lround(cost.amount));
		switch (cost.type) {
		case ActionResourceType::Hp:
		{
			const int32_t previousHp = stats->currentHp;
			stats->currentHp =
				std::clamp(stats->currentHp - amount, 0, stats->maxHp);
			statDirty = statDirty || previousHp != stats->currentHp;
			break;
		}

		case ActionResourceType::Stamina:
		{
			const int32_t previousStamina = stats->currentStamina;
			stats->currentStamina =
				std::clamp(stats->currentStamina - amount, 0, stats->maxStamina);
			statDirty =
				statDirty || previousStamina != stats->currentStamina;
			break;
		}

		default:
			break;
		}
	}

	if (!statDirty)
	{
		return;
	}

	if (DirtyFlagsComp* dirty = ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
	{
		dirty->MarkDirty(WorldDirtyType::Stat);
	}
}

void ResolveActionStateSystem::ClearActionInput(ActorInputComp& input)
{
	input.action = {};
}

std::vector<ResolveActionStateSystem::RequestCandidate>
ResolveActionStateSystem::BuildRequestCandidates(
	const ActionProfileService& profileService,
	CharacterId characterId,
	const ActorInputComp& input,
	bool includeHeldGuardRequest)
{
	std::vector<RequestCandidate> candidates;

	if (input.action.directActionId != ActionId::None)
	{
		candidates.push_back(RequestCandidate{
			input.action.directActionId,
			input.action.directionX,
			input.action.directionZ,
			true
		});
		return candidates;
	}

	ActionRequestSemantic semantic = ActionRequestSemantic::None;
	switch (input.action.type) {
	case PlayerActionInputType::LightAttack:
		semantic = ActionRequestSemantic::LightAttack;
		break;
	case PlayerActionInputType::HeavyAttack:
		semantic = ActionRequestSemantic::HeavyAttack;
		break;
	case PlayerActionInputType::Dodge:
		semantic = ActionRequestSemantic::Dodge;
		break;
	case PlayerActionInputType::Parry:
		semantic = ActionRequestSemantic::Parry;
		break;
	case PlayerActionInputType::None:
	default:
		semantic = ActionRequestSemantic::None;
		break;
	}

	if (semantic == ActionRequestSemantic::None &&
		includeHeldGuardRequest &&
		input.guard.isPressed)
	{
		semantic = ActionRequestSemantic::GuardStart;
	}

	if (semantic == ActionRequestSemantic::None)
	{
		return candidates;
	}

	const ActionInputBindingProfileDef* bindingProfile =
		profileService.FindInputBindingProfile(characterId);
	if (bindingProfile == nullptr)
	{
		return candidates;
	}

	const ActionInputBindingEntryDef* selectedEntry = nullptr;
	for (const ActionInputBindingEntryDef& entry : bindingProfile->entries)
	{
		if (entry.request != semantic || entry.candidateActions.empty())
		{
			continue;
		}

		if (selectedEntry == nullptr || entry.priority > selectedEntry->priority)
		{
			selectedEntry = &entry;
		}
	}

	if (selectedEntry == nullptr)
	{
		return candidates;
	}

	candidates.reserve(selectedEntry->candidateActions.size());
	for (ActionId actionId : selectedEntry->candidateActions)
	{
		candidates.push_back(RequestCandidate{
			actionId,
			input.action.directionX,
			input.action.directionZ,
			true
		});
	}

	return candidates;
}

bool ResolveActionStateSystem::IsActionRequestAllowed(
	ActionId actionId,
	const CombatStatStateComp* stats,
	const AIPerceptionComp* perception)
{
	const ActionDef* actionDef = FindActionDef(actionId);
	if (actionDef == nullptr)
	{
		return false;
	}

	for (const ActionRequestRequirementDef& requirement :
		actionDef->requestRequirements)
	{
		switch (requirement.type) {
		case ActionRequestRequirementType::None:
			break;

		case ActionRequestRequirementType::HasEnoughStamina:
			if (stats == nullptr ||
				stats->currentStamina <
					static_cast<int32_t>(requirement.scalar.value_or(0.0f)))
			{
				return false;
			}
			break;

		case ActionRequestRequirementType::HasTarget:
			if (perception == nullptr ||
				!perception->hasTarget ||
				perception->selectedTarget.IsNull())
			{
				return false;
			}
			break;

		case ActionRequestRequirementType::IsGrounded:
			// Grounding state is not modeled separately yet.
			break;

		case ActionRequestRequirementType::HasStateFlag:
			return false;

		case ActionRequestRequirementType::MissingStateFlag:
			break;

		default:
			return false;
		}
	}

	return true;
}

bool ResolveActionStateSystem::IsCancelRuleActive(
	const ActionCancelRule& cancelRule,
	const ActionDef& currentActionDef,
	const ActionStateComp& actionState)
{
	if (cancelRule.windowPolicy == ActionWindowPolicy::Always)
	{
		return true;
	}

	const float progress =
		(currentActionDef.duration > 0.0f)
		? ClampFloat(actionState.elapsedSec / currentActionDef.duration, 0.0f, 1.0f)
		: 1.0f;
	const float windowStart = cancelRule.windowStartNormalized.value_or(0.0f);
	const float windowEnd = cancelRule.windowEndNormalized.value_or(1.0f);
	return progress >= windowStart && progress <= windowEnd;
}

bool ResolveActionStateSystem::IsHoldReleased(
	const ActionDef& actionDef,
	const ActorInputComp& input)
{
	return
		actionDef.normalizedPolicy == ActionNormalizedPolicy::Holdable &&
		actionDef.endPolicy.endType == ActionEndType::HoldRelease &&
		actionDef.kind == ActionKind::Guard &&
		!input.guard.isPressed;
}

float ResolveActionStateSystem::ComputeAdvancedElapsedSec(
	const ActionStateComp& actionState,
	const ActionDef& actionDef,
	double deltaTimeSec)
{
	constexpr float kHoldActionElapsedEpsilonSec = 1.0e-4f;

	float nextElapsedSec =
		actionState.elapsedSec + static_cast<float>(deltaTimeSec);

	if (actionDef.normalizedPolicy == ActionNormalizedPolicy::Holdable &&
		actionDef.endPolicy.endType == ActionEndType::HoldRelease)
	{
		const float holdElapsedSec =
			std::max(0.0f, actionDef.duration - kHoldActionElapsedEpsilonSec);
		nextElapsedSec = std::min(nextElapsedSec, holdElapsedSec);
	}
	else if (actionDef.kind == ActionKind::Dead)
	{
		nextElapsedSec = std::min(nextElapsedSec, actionDef.duration);
	}

	return nextElapsedSec;
}

void ResolveActionStateSystem::PrepareTimelineAdvance(
	const ActionStateComp& actionState,
	const ActionDef& actionDef,
	ActionTimelineAdvanceComp& advance,
	double deltaTimeSec)
{
	advance.actionId = actionState.actionId;
	advance.actionInstanceId = actionState.actionInstanceId;
	advance.prevElapsedSec = actionState.elapsedSec;
	advance.currElapsedSec =
		ComputeAdvancedElapsedSec(actionState, actionDef, deltaTimeSec);
	advance.startedThisFrame = false;
}

void ResolveActionStateSystem::PrepareStartedActionAdvance(
	const ActionStateComp& actionState,
	ActionTimelineAdvanceComp& advance)
{
	advance.actionId = actionState.actionId;
	advance.actionInstanceId = actionState.actionInstanceId;
	advance.prevElapsedSec = 0.0f;
	advance.currElapsedSec = 0.0f;
	advance.startedThisFrame = true;
}

void ResolveActionStateSystem::SetActionDirectionOnStart(
	ActionStateComp& actionState,
	const TransitionDecision& decision,
	const LocomotionStateComp& locomotionState,
	const WorldTransformComp& transform)
{
	if (!decision.preserveDirection)
	{
		return;
	}

	float dirX = decision.directionX;
	float dirZ = decision.directionZ;
	NormalizeXZ(dirX, dirZ);

	if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
	{
		dirX = locomotionState.desiredMoveDirX;
		dirZ = locomotionState.desiredMoveDirZ;
		NormalizeXZ(dirX, dirZ);
	}

	if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
	{
		const XMVECTOR forward = TransformHelper::Forward(transform);
		XMFLOAT3 dir{};
		XMStoreFloat3(&dir, forward);

		dirX = dir.x;
		dirZ = dir.z;
		NormalizeXZ(dirX, dirZ);
	}

	actionState.directionX = dirX;
	actionState.directionZ = dirZ;
}

void ResolveActionStateSystem::ResetActionDirection(ActionStateComp& actionState)
{
	actionState.directionX = 0.0f;
	actionState.directionZ = 0.0f;
}
