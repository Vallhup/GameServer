#include "pch.h"
#include "ResolveAbilityStateSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"
#include "../../../GameplayContentCatalog.h"
#include "../Phase1/ApplyAICommandSystem.h"
#include "../Phase1/ApplyPlayerCommandSystem.h"

#include <limits>

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<12, 0, 2> ResolveAbilityStateSystem::kMetaStorage =
    MakeMetaStorage(
        SysTag<ResolveAbilityStateSystem>(),
        "ResolveAbilityStateSystem",
        std::array<AccessSpec, 12>
        {
            WriteImmediate(ComponentRes<AbilityStateComp>()),
            ReadImmediate(ComponentRes<LocomotionStateComp>()),
            ReadImmediate(ComponentRes<WorldTransformComp>()),
            WriteImmediate(ComponentRes<ActorInputComp>()),
            WriteImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
            ReadImmediate(ComponentRes<SpawnTypeComp>()),
            WriteImmediate(ComponentRes<CombatStatStateComp>()),
            WriteImmediate(ComponentRes<AbilityInterruptQueueComp>()),
            WriteImmediate(ComponentRes<DirtyFlagsComp>()),
            ReadImmediate(ComponentRes<AIPerceptionComp>()),
            ReadImmediate(ComponentRes<PendingDespawnTag>()),
            ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
        },
        std::array<SystemTag, 0>{},
        std::array<SystemTag, 2>
        {
            SysTag<ApplyAICommandSystem>(),
            SysTag<ApplyPlayerCommandSystem>(),
        });

void ResolveAbilityStateSystem::Execute(SystemContext& ctx)
{
	const AbilityProfileService profileService{};

	for (auto [
		entity,
		abilityState,
		locomotionState,
		transform,
		input,
		advance,
		spawnType] :
		ctx.ecs.View<
			AbilityStateComp,
			LocomotionStateComp,
			WorldTransformComp,
			ActorInputComp,
			AbilityTimelineAdvanceComp,
			SpawnTypeComp>())
	{
		ClearAbilityTimelineAdvance(advance);

		if (TryHandleBlockingState(
			ctx,
			entity,
			abilityState,
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
		const AbilityInterruptQueueComp* interruptQueue =
			ctx.ecs.GetComponent<AbilityInterruptQueueComp>(entity);

		const AbilityDef* currentAbilityDef =
			IsAbilityActive(abilityState)
			? GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId)
			: nullptr;

		if (IsAbilityActive(abilityState) && currentAbilityDef == nullptr)
		{
			abilityState = {};
			ClearAbilityInput(input);
			continue;
		}

		TransitionDecision decision{};
		const bool hadInterruptEvents =
			interruptQueue != nullptr && !interruptQueue->events.empty();
		if (hadInterruptEvents &&
			TryResolveInterruptTransition(
				profileService,
				characterId,
				abilityState,
				currentAbilityDef,
				interruptQueue,
				decision))
		{
			ApplyTransition(abilityState, decision);
			ConsumeOnRequestResourceCosts(ctx, entity, decision);
			if (IsAbilityActive(abilityState))
			{
				PrepareStartedAbilityAdvance(abilityState, advance);
			}
			if (AbilityInterruptQueueComp* mutableInterruptQueue =
				ctx.ecs.GetMutableComponent<AbilityInterruptQueueComp>(entity))
			{
				mutableInterruptQueue->events.clear();
			}
			ClearAbilityInput(input);
			continue;
		}

		if (hadInterruptEvents)
		{
			if (AbilityInterruptQueueComp* mutableInterruptQueue =
				ctx.ecs.GetMutableComponent<AbilityInterruptQueueComp>(entity))
			{
				mutableInterruptQueue->events.clear();
			}
		}

		if (currentAbilityDef != nullptr)
		{
			if (TryResolveCancelTransition(
				profileService,
				characterId,
				abilityState,
				*currentAbilityDef,
				locomotionState,
				transform,
				input,
				stats,
				perception,
				decision))
			{
				ApplyTransition(abilityState, decision);
				ConsumeOnRequestResourceCosts(ctx, entity, decision);
				SetAbilityDirectionOnStart(
					abilityState,
					decision,
					locomotionState,
					transform);
				if (IsAbilityActive(abilityState))
				{
					PrepareStartedAbilityAdvance(abilityState, advance);
				}
				ClearAbilityInput(input);
				continue;
			}

			(void)TryResolveEndPolicyTransition(
				abilityState,
				*currentAbilityDef,
				input,
				advance,
				ctx.dtSec,
				decision);
			if (decision.transition)
			{
				ApplyTransition(abilityState, decision);
				ConsumeOnRequestResourceCosts(ctx, entity, decision);
				if (IsAbilityActive(abilityState))
				{
					PrepareStartedAbilityAdvance(abilityState, advance);
				}
			}

			ClearAbilityInput(input);
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
			ApplyTransition(abilityState, decision);
			ConsumeOnRequestResourceCosts(ctx, entity, decision);
			SetAbilityDirectionOnStart(
				abilityState,
				decision,
				locomotionState,
				transform);
			if (IsAbilityActive(abilityState))
			{
				PrepareStartedAbilityAdvance(abilityState, advance);
			}
		}

		ClearAbilityInput(input);
	}
}

bool ResolveAbilityStateSystem::TryHandleBlockingState(
	SystemContext& ctx,
	Entity entity,
	AbilityStateComp& abilityState,
	ActorInputComp& input,
	AbilityTimelineAdvanceComp& advance)
{
	if (!HasBlockingPendingState(ctx.ecs, entity))
	{
		return false;
	}

	ClearAbilityTimelineAdvance(advance);
	abilityState.abilityId = InvalidAbilityId;
	abilityState.elapsedSec = 0.0f;
	ResetAbilityDirection(abilityState);
	ClearAbilityInput(input);
	return true;
}

bool ResolveAbilityStateSystem::TryResolveInterruptTransition(
	const AbilityProfileService& profileService,
	const CharacterId characterId,
	const AbilityStateComp& abilityState,
	const AbilityDef* currentAbilityDef,
	const AbilityInterruptQueueComp* interruptQueue,
	TransitionDecision& outDecision)
{
	if (interruptQueue == nullptr || interruptQueue->events.empty())
	{
		return false;
	}

	int bestPriority = std::numeric_limits<int>::min();
	AbilityId bestAbilityId = InvalidAbilityId;

	if (currentAbilityDef != nullptr)
	{
		const float progress =
			(currentAbilityDef->timeline.durationSec > 0.0f)
			? ClampFloat(abilityState.elapsedSec / currentAbilityDef->timeline.durationSec, 0.0f, 1.0f)
			: 1.0f;

		for (const AbilityInterruptEvent& event : interruptQueue->events)
		{
			for (const AbilityTransitionRuleDef& rule :
				currentAbilityDef->transition.interruptRules)
			{
				if (rule.cause != event.cause)
				{
					continue;
				}

				if (rule.windowPolicy == AbilityTransitionWindowPolicy::Range)
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
					if (!profileService.IsAbilityAvailable(characterId, rule.toAbilityId))
					{
						continue;
					}

					bestPriority = rule.priority;
					bestAbilityId = rule.toAbilityId;
				}
			}
		}
	}
	else
	{
		const AbilityFallbackReactionProfileDef* fallbackProfile =
			profileService.FindFallbackReactionProfile(characterId);
		if (fallbackProfile == nullptr)
		{
			return false;
		}

		for (const AbilityInterruptEvent& event : interruptQueue->events)
		{
			for (const AbilityFallbackReactionEntryDef& entry :
				fallbackProfile->entries)
			{
				if (entry.cause != event.cause)
				{
					continue;
				}

				if (entry.priority > bestPriority)
				{
					if (!profileService.IsAbilityAvailable(characterId, entry.toAbilityId))
					{
						continue;
					}

					bestPriority = entry.priority;
					bestAbilityId = entry.toAbilityId;
				}
			}
		}
	}

	if (bestAbilityId == InvalidAbilityId)
	{
		return false;
	}

	outDecision.transition = true;
	outDecision.nextAbilityId = bestAbilityId;
	outDecision.preserveDirection = false;
	return true;
}

bool ResolveAbilityStateSystem::TryResolveCancelTransition(
	const AbilityProfileService& profileService,
	const CharacterId characterId,
	const AbilityStateComp& abilityState,
	const AbilityDef& currentAbilityDef,
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
		if (candidate.abilityId == InvalidAbilityId ||
			!profileService.IsAbilityAvailable(characterId, candidate.abilityId) ||
			!IsAbilityRequestAllowed(candidate.abilityId, stats, perception))
		{
			continue;
		}

		for (const AbilityTransitionRuleDef& cancelRule :
			currentAbilityDef.transition.cancelRules)
		{
			if (cancelRule.toAbilityId != candidate.abilityId ||
				!IsCancelRuleActive(cancelRule, currentAbilityDef, abilityState))
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
	outDecision.nextAbilityId = bestCandidate.abilityId;
	outDecision.consumeOnRequestCosts = true;
	outDecision.preserveDirection = true;
	outDecision.target = bestCandidate.target;
	outDecision.directionX = bestCandidate.directionX;
	outDecision.directionZ = bestCandidate.directionZ;
	(void)locomotionState;
	(void)transform;
	return true;
}

bool ResolveAbilityStateSystem::TryResolveIdleRequestTransition(
	const AbilityProfileService& profileService,
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
		if (candidate.abilityId == InvalidAbilityId ||
			!profileService.IsAbilityAvailable(characterId, candidate.abilityId) ||
			!IsAbilityRequestAllowed(candidate.abilityId, stats, perception))
		{
			continue;
		}

		outDecision.transition = true;
		outDecision.nextAbilityId = candidate.abilityId;
		outDecision.consumeOnRequestCosts = true;
		outDecision.preserveDirection = true;
		outDecision.target = candidate.target;
		outDecision.directionX = candidate.directionX;
		outDecision.directionZ = candidate.directionZ;
		(void)locomotionState;
		(void)transform;
		return true;
	}

	return false;
}

bool ResolveAbilityStateSystem::TryResolveEndPolicyTransition(
	const AbilityStateComp& abilityState,
	const AbilityDef& abilityDef,
	const ActorInputComp& input,
	AbilityTimelineAdvanceComp& advance,
	double deltaTimeSec,
	TransitionDecision& outDecision)
{
	if (IsHoldReleased(abilityDef, input))
	{
		outDecision.transition = true;
		outDecision.nextAbilityId = abilityDef.transition.endPolicy.defaultNextAbilityId;
		outDecision.preserveDirection = false;
		return true;
	}

	PrepareTimelineAdvance(abilityState, abilityDef, advance, deltaTimeSec);

	if (abilityDef.timeline.policy == AbilityTimelinePolicy::Holdable &&
		abilityDef.transition.endPolicy.endType == AbilityEndType::HoldRelease)
	{
		return false;
	}

	if (advance.currElapsedSec < abilityDef.timeline.durationSec)
	{
		return false;
	}

	if (abilityDef.kind == AbilityKind::Dead)
	{
		return false;
	}

	outDecision.transition = true;
	outDecision.nextAbilityId = abilityDef.transition.endPolicy.defaultNextAbilityId;
	outDecision.preserveDirection = false;
	return true;
}

void ResolveAbilityStateSystem::ApplyTransition(
	AbilityStateComp& abilityState,
	const TransitionDecision& decision)
{
	if (!decision.transition)
	{
		return;
	}

	++abilityState.abilityInstanceId;
	abilityState.abilityId = decision.nextAbilityId;
	abilityState.elapsedSec = 0.0f;
	ResetAbilityDirection(abilityState);
}

void ResolveAbilityStateSystem::ConsumeOnRequestResourceCosts(
	SystemContext& ctx,
	Entity entity,
	const TransitionDecision& decision)
{
	if (!decision.transition || !decision.consumeOnRequestCosts)
	{
		return;
	}

	const AbilityDef* abilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(decision.nextAbilityId);
	if (abilityDef == nullptr)
	{
		return;
	}

	CombatStatStateComp* stats =
		ctx.ecs.GetMutableComponent<CombatStatStateComp>(entity);
	if (stats == nullptr)
	{
		return;
	}

	const GameplayContentCatalogSnapshot& contentCatalog =
		GameplayContentCatalogSnapshot::Current();
	const AttributeDef* hpAttribute =
		contentCatalog.FindAttributeByKey("Attribute.Hp");
	const AttributeDef* staminaAttribute =
		contentCatalog.FindAttributeByKey("Attribute.Stamina");

	bool statDirty = false;
	for (const AbilityCostDef& cost : abilityDef->costs)
	{
		if (cost.consumeTiming != AbilityCostConsumeTiming::OnRequest)
		{
			continue;
		}

		const int32_t amount = static_cast<int32_t>(std::lround(cost.amount));
		if (hpAttribute != nullptr && cost.attributeId == hpAttribute->id)
		{
			const int32_t previousHp = stats->currentHp;
			stats->currentHp =
				std::clamp(stats->currentHp - amount, 0, stats->maxHp);
			statDirty = statDirty || previousHp != stats->currentHp;
			continue;
		}

		if (staminaAttribute != nullptr &&
			cost.attributeId == staminaAttribute->id)
		{
			const int32_t previousStamina = stats->currentStamina;
			stats->currentStamina =
				std::clamp(stats->currentStamina - amount, 0, stats->maxStamina);
			statDirty =
				statDirty || previousStamina != stats->currentStamina;
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

void ResolveAbilityStateSystem::ClearAbilityInput(ActorInputComp& input)
{
	input.ability = {};
}

std::vector<ResolveAbilityStateSystem::RequestCandidate>
ResolveAbilityStateSystem::BuildRequestCandidates(
	const AbilityProfileService& profileService,
	CharacterId characterId,
	const ActorInputComp& input,
	bool includeHeldGuardRequest)
{
	std::vector<RequestCandidate> candidates;

	if (input.ability.directAbilityId != InvalidAbilityId)
	{
		candidates.push_back(RequestCandidate{
			input.ability.directAbilityId,
			input.ability.target,
			input.ability.directionX,
			input.ability.directionZ,
			true
		});
		return candidates;
	}

	AbilityRequestSemantic semantic = AbilityRequestSemantic::None;
	switch (input.ability.type) {
	case PlayerAbilityInputType::LightAttack:
		semantic = AbilityRequestSemantic::LightAttack;
		break;
	case PlayerAbilityInputType::HeavyAttack:
		semantic = AbilityRequestSemantic::HeavyAttack;
		break;
	case PlayerAbilityInputType::Dodge:
		semantic = AbilityRequestSemantic::Dodge;
		break;
	case PlayerAbilityInputType::Parry:
		semantic = AbilityRequestSemantic::Parry;
		break;
	case PlayerAbilityInputType::None:
	default:
		semantic = AbilityRequestSemantic::None;
		break;
	}

	if (semantic == AbilityRequestSemantic::None &&
		includeHeldGuardRequest &&
		input.guard.isPressed)
	{
		semantic = AbilityRequestSemantic::GuardStart;
	}

	if (semantic == AbilityRequestSemantic::None)
	{
		return candidates;
	}

	const AbilityInputBindingProfileDef* bindingProfile =
		profileService.FindInputBindingProfile(characterId);
	if (bindingProfile == nullptr)
	{
		return candidates;
	}

	const AbilityInputBindingEntryDef* selectedEntry = nullptr;
	for (const AbilityInputBindingEntryDef& entry : bindingProfile->entries)
	{
		if (entry.request != semantic || entry.candidateAbilities.empty())
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

	candidates.reserve(selectedEntry->candidateAbilities.size());
	for (AbilityId abilityId : selectedEntry->candidateAbilities)
	{
		candidates.push_back(RequestCandidate{
			abilityId,
			input.ability.target,
			input.ability.directionX,
			input.ability.directionZ,
			true
		});
	}

	return candidates;
}

bool ResolveAbilityStateSystem::IsAbilityRequestAllowed(
	AbilityId abilityId,
	const CombatStatStateComp* stats,
	const AIPerceptionComp* perception)
{
	const AbilityDef* abilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityId);
	if (abilityDef == nullptr)
	{
		return false;
	}

	const AttributeDef* staminaAttribute =
		GameplayContentCatalogSnapshot::Current()
			.FindAttributeByKey("Attribute.Stamina");

	for (const AbilityCostDef& cost : abilityDef->costs)
	{
		if (cost.consumeTiming != AbilityCostConsumeTiming::OnRequest)
			continue;

		if (stats != nullptr &&
			staminaAttribute != nullptr &&
			cost.attributeId == staminaAttribute->id &&
			stats->currentStamina <
				static_cast<int32_t>(std::lround(cost.amount)))
		{
			return false;
		}
	}

	if (abilityDef->activation.requiresTarget &&
		(perception == nullptr ||
			!perception->hasTarget ||
			perception->selectedTarget.IsNull()))
	{
		return false;
	}

	return true;
}

bool ResolveAbilityStateSystem::IsCancelRuleActive(
	const AbilityTransitionRuleDef& cancelRule,
	const AbilityDef& currentAbilityDef,
	const AbilityStateComp& abilityState)
{
	if (cancelRule.windowPolicy == AbilityTransitionWindowPolicy::Always)
	{
		return true;
	}

	const float progress =
		(currentAbilityDef.timeline.durationSec > 0.0f)
		? ClampFloat(abilityState.elapsedSec / currentAbilityDef.timeline.durationSec, 0.0f, 1.0f)
		: 1.0f;
	const float windowStart = cancelRule.windowStartNormalized.value_or(0.0f);
	const float windowEnd = cancelRule.windowEndNormalized.value_or(1.0f);
	return progress >= windowStart && progress <= windowEnd;
}

bool ResolveAbilityStateSystem::IsHoldReleased(
	const AbilityDef& abilityDef,
	const ActorInputComp& input)
{
	return
		abilityDef.timeline.policy == AbilityTimelinePolicy::Holdable &&
		abilityDef.transition.endPolicy.endType == AbilityEndType::HoldRelease &&
		abilityDef.kind == AbilityKind::Guard &&
		!input.guard.isPressed;
}

float ResolveAbilityStateSystem::ComputeAdvancedElapsedSec(
	const AbilityStateComp& abilityState,
	const AbilityDef& abilityDef,
	double deltaTimeSec)
{
	constexpr float kHoldAbilityElapsedEpsilonSec = 1.0e-4f;

	float nextElapsedSec =
		abilityState.elapsedSec + static_cast<float>(deltaTimeSec);

	if (abilityDef.timeline.policy == AbilityTimelinePolicy::Holdable &&
		abilityDef.transition.endPolicy.endType == AbilityEndType::HoldRelease)
	{
		const float holdElapsedSec =
			std::max(0.0f, abilityDef.timeline.durationSec - kHoldAbilityElapsedEpsilonSec);
		nextElapsedSec = std::min(nextElapsedSec, holdElapsedSec);
	}
	else if (abilityDef.kind == AbilityKind::Dead)
	{
		nextElapsedSec = std::min(nextElapsedSec, abilityDef.timeline.durationSec);
	}

	return nextElapsedSec;
}

void ResolveAbilityStateSystem::PrepareTimelineAdvance(
	const AbilityStateComp& abilityState,
	const AbilityDef& abilityDef,
	AbilityTimelineAdvanceComp& advance,
	double deltaTimeSec)
{
	advance.abilityId = abilityState.abilityId;
	advance.abilityInstanceId = abilityState.abilityInstanceId;
	advance.prevElapsedSec = abilityState.elapsedSec;
	advance.currElapsedSec =
		ComputeAdvancedElapsedSec(abilityState, abilityDef, deltaTimeSec);
	advance.startedThisFrame = false;
}

void ResolveAbilityStateSystem::PrepareStartedAbilityAdvance(
	const AbilityStateComp& abilityState,
	AbilityTimelineAdvanceComp& advance)
{
	advance.abilityId = abilityState.abilityId;
	advance.abilityInstanceId = abilityState.abilityInstanceId;
	advance.prevElapsedSec = 0.0f;
	advance.currElapsedSec = 0.0f;
	advance.startedThisFrame = true;
}

void ResolveAbilityStateSystem::SetAbilityDirectionOnStart(
	AbilityStateComp& abilityState,
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

	abilityState.directionX = dirX;
	abilityState.directionZ = dirZ;
	abilityState.target = decision.target;
}

void ResolveAbilityStateSystem::ResetAbilityDirection(AbilityStateComp& abilityState)
{
	abilityState.directionX = 0.0f;
	abilityState.directionZ = 0.0f;
	abilityState.target = Entity::Null();
}
