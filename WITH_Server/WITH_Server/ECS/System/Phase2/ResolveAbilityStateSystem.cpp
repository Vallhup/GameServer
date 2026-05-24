#include "pch.h"
#include "ResolveAbilityStateSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"
#include "../../../GameplayContentCatalog.h"
#include "../Phase1/ApplyAICommandSystem.h"
#include "../Phase1/ApplyPlayerCommandSystem.h"

#include <cmath>
#include <limits>

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<16, 0, 2> ResolveAbilityStateSystem::kMetaStorage =
    MakeMetaStorage(
        SysTag<ResolveAbilityStateSystem>(),
        "ResolveAbilityStateSystem",
        std::array<AccessSpec, 16>
        {
            WriteImmediate(ComponentRes<AbilityStateComp>()),
            ReadImmediate(ComponentRes<LocomotionStateComp>()),
            ReadImmediate(ComponentRes<WorldTransformComp>()),
            WriteImmediate(ComponentRes<ActorInputComp>()),
            ReadImmediate(ComponentRes<PlayerNetworkCompensationComp>()),
            WriteImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
            ReadImmediate(ComponentRes<SpawnTypeComp>()),
            WriteImmediate(ComponentRes<CombatStatStateComp>()),
            WriteImmediate(ComponentRes<StaminaRecoveryStateComp>()),
            WriteImmediate(ComponentRes<AbilityInterruptQueueComp>()),
            WriteImmediate(ComponentRes<DirtyFlagsComp>()),
            ReadImmediate(ComponentRes<AIPerceptionComp>()),
            ReadImmediate(ComponentRes<PendingDespawnTag>()),
            ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
            ReadImmediate(ComponentRes<ConsumableInventoryComp>()),
            ReadImmediate(ComponentRes<GameplayTagStateComp>()),
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
		const PlayerNetworkCompensationComp* networkCompensation =
			ctx.ecs.GetComponent<PlayerNetworkCompensationComp>(entity);
		const float abilityInputBufferDurationSec =
			ResolveAbilityInputBufferDurationSec(networkCompensation);
		UpdateAbilityInputBuffer(input, ctx.dtSec);

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
		const ConsumableInventoryComp* inventory =
			ctx.ecs.GetComponent<ConsumableInventoryComp>(entity);
		const AIPerceptionComp* perception =
			ctx.ecs.GetComponent<AIPerceptionComp>(entity);
		const GameplayTagStateComp* tags =
			ctx.ecs.GetComponent<GameplayTagStateComp>(entity);
		const AbilityInterruptQueueComp* interruptQueue =
			ctx.ecs.GetComponent<AbilityInterruptQueueComp>(entity);

		const AbilityDef* currentAbilityDef =
			IsAbilityActive(abilityState)
			? GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId)
			: nullptr;

		if (IsAbilityActive(abilityState) && currentAbilityDef == nullptr)
		{
			abilityState = {};
			ClearAllAbilityInput(input);
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
			ClearAllAbilityInput(input);
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
				inventory,
				perception,
				tags,
				networkCompensation,
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
				FinishAbilityInput(
					input,
					decision.consumeAbilityInput,
					false,
					abilityInputBufferDurationSec);
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

			FinishAbilityInput(
				input,
				false,
				true,
				abilityInputBufferDurationSec);
			continue;
		}

		if (TryResolveIdleRequestTransition(
			profileService,
			characterId,
			locomotionState,
			transform,
			input,
			stats,
			inventory,
			perception,
			tags,
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

		FinishAbilityInput(
			input,
			decision.consumeAbilityInput,
			false,
			abilityInputBufferDurationSec);
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
	ClearAllAbilityInput(input);
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
	const ConsumableInventoryComp* inventory,
	const AIPerceptionComp* perception,
	const GameplayTagStateComp* tags,
	const PlayerNetworkCompensationComp* networkCompensation,
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
			!IsAbilityStartLocomotionAllowed(candidate.abilityId, locomotionState.mode) ||
			!IsAbilityRequestAllowed(candidate.abilityId, stats, inventory, perception, tags))
		{
			continue;
		}

		for (const AbilityTransitionRuleDef& cancelRule :
			currentAbilityDef.transition.cancelRules)
		{
			if (cancelRule.toAbilityId != candidate.abilityId ||
				!IsCancelRuleActive(
					cancelRule,
					currentAbilityDef,
					abilityState,
					characterId,
					candidate.request,
					candidate.fromBufferedInput,
					networkCompensation))
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
	outDecision.consumeAbilityInput = true;
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
	const ConsumableInventoryComp* inventory,
	const AIPerceptionComp* perception,
	const GameplayTagStateComp* tags,
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
			!IsAbilityStartLocomotionAllowed(candidate.abilityId, locomotionState.mode) ||
			!IsAbilityRequestAllowed(candidate.abilityId, stats, inventory, perception, tags))
		{
			continue;
		}

		outDecision.transition = true;
		outDecision.nextAbilityId = candidate.abilityId;
		outDecision.consumeOnRequestCosts = true;
		outDecision.preserveDirection = true;
		outDecision.consumeAbilityInput = true;
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
			if (previousStamina > stats->currentStamina)
			{
				StaminaRecoveryStateComp* recovery =
					ctx.ecs.GetMutableComponent<StaminaRecoveryStateComp>(entity);
				ApplyStaminaRecoveryDelay(
					ctx,
					entity,
					*stats,
					recovery != nullptr
						? recovery->tuning.spendRegenDelaySec
						: StaminaRecoveryTuning{}.spendRegenDelaySec);
			}
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

void ResolveAbilityStateSystem::ApplyStaminaRecoveryDelay(
	SystemContext& ctx,
	Entity entity,
	const CombatStatStateComp& stats,
	float delaySec)
{
	StaminaRecoveryStateComp* recovery =
		ctx.ecs.GetMutableComponent<StaminaRecoveryStateComp>(entity);
	if (recovery == nullptr)
	{
		return;
	}

	if (stats.currentStamina <= 0)
	{
		delaySec = std::max(delaySec, recovery->tuning.exhaustedRegenDelaySec);
	}

	recovery->regenLockRemainingSec =
		std::max(recovery->regenLockRemainingSec, delaySec);
}

void ResolveAbilityStateSystem::UpdateAbilityInputBuffer(
	ActorInputComp& input,
	double deltaTimeSec)
{
	if (!input.abilityBuffer.hasEvent)
	{
		return;
	}

	input.abilityBuffer.remainingSec =
		std::max(
			0.0f,
			input.abilityBuffer.remainingSec -
			static_cast<float>(deltaTimeSec));
	if (input.abilityBuffer.remainingSec <= 0.0f)
	{
		ClearAbilityInputBuffer(input);
	}
}

bool ResolveAbilityStateSystem::HasAbilityInputRequest(
	const ActorAbilityInputEvent& input) noexcept
{
	return
		input.type != PlayerAbilityInputType::None ||
		input.directAbilityId != InvalidAbilityId;
}

bool ResolveAbilityStateSystem::CanBufferAbilityInput(
	const ActorAbilityInputEvent& input) noexcept
{
	return
		input.type != PlayerAbilityInputType::None &&
		input.directAbilityId == InvalidAbilityId;
}

void ResolveAbilityStateSystem::BufferCurrentAbilityInput(
	ActorInputComp& input,
	float bufferDurationSec)
{
	if (!CanBufferAbilityInput(input.ability))
	{
		return;
	}

	input.abilityBuffer.event = input.ability;
	input.abilityBuffer.remainingSec = bufferDurationSec;
	input.abilityBuffer.hasEvent = true;
}

void ResolveAbilityStateSystem::ClearAbilityInput(ActorInputComp& input)
{
	input.ability = {};
}

void ResolveAbilityStateSystem::ClearAbilityInputBuffer(ActorInputComp& input)
{
	input.abilityBuffer = {};
}

void ResolveAbilityStateSystem::ClearAllAbilityInput(ActorInputComp& input)
{
	ClearAbilityInput(input);
	ClearAbilityInputBuffer(input);
}

void ResolveAbilityStateSystem::FinishAbilityInput(
	ActorInputComp& input,
	bool consumed,
	bool allowBuffering,
	float bufferDurationSec)
{
	if (consumed)
	{
		ClearAllAbilityInput(input);
		return;
	}

	if (allowBuffering)
	{
		BufferCurrentAbilityInput(input, bufferDurationSec);
	}

	ClearAbilityInput(input);
}

std::vector<ResolveAbilityStateSystem::RequestCandidate>
ResolveAbilityStateSystem::BuildRequestCandidates(
	const AbilityProfileService& profileService,
	CharacterId characterId,
	const ActorInputComp& input,
	bool includeHeldGuardRequest)
{
	std::vector<RequestCandidate> candidates;
	const ActorAbilityInputEvent* abilityInput = &input.ability;
	bool fromBufferedInput = false;

	if (!HasAbilityInputRequest(*abilityInput) &&
		input.abilityBuffer.hasEvent)
	{
		abilityInput = &input.abilityBuffer.event;
		fromBufferedInput = true;
	}

	if (abilityInput->directAbilityId != InvalidAbilityId)
	{
		candidates.push_back(RequestCandidate{
			abilityInput->directAbilityId,
			abilityInput->target,
			abilityInput->directionX,
			abilityInput->directionZ,
			true,
			*abilityInput,
			fromBufferedInput
		});
		return candidates;
	}

	AbilityRequestSemantic semantic = AbilityRequestSemantic::None;
	switch (abilityInput->type) {
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
	case PlayerAbilityInputType::UseItem:
		semantic = AbilityRequestSemantic::UseItem;
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
			abilityInput->target,
			abilityInput->directionX,
			abilityInput->directionZ,
			true,
			*abilityInput,
			fromBufferedInput
		});
	}

	return candidates;
}

bool ResolveAbilityStateSystem::IsAbilityRequestAllowed(
	AbilityId abilityId,
	const CombatStatStateComp* stats,
	const ConsumableInventoryComp* inventory,
	const AIPerceptionComp* perception,
	const GameplayTagStateComp* tags)
{
	const AbilityDef* abilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityId);
	if (abilityDef == nullptr)
	{
		return false;
	}

	const GameplayTagMask ownerTags = BuildGameplayTagMask(tags);
	if (!EvaluateGameplayTagQuery(
			abilityDef->activation.requiredOwnerTags,
			ownerTags,
			true))
	{
		return false;
	}
	if (EvaluateGameplayTagQuery(
			abilityDef->activation.blockedOwnerTags,
			ownerTags,
			false))
	{
		return false;
	}

	if (abilityDef->kind == AbilityKind::UseItem)
	{
		if (stats == nullptr ||
			inventory == nullptr ||
			inventory->hpPotionCount == 0 ||
			stats->currentHp <= 0 ||
			stats->maxHp <= 0 ||
			stats->currentHp >= stats->maxHp)
		{
			return false;
		}
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

bool ResolveAbilityStateSystem::IsAbilityStartLocomotionAllowed(
	AbilityId abilityId,
	LocomotionMode locomotionMode)
{
	const AbilityDef* abilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityId);
	if (abilityDef == nullptr)
	{
		return false;
	}

	if (abilityDef->kind != AbilityKind::UseItem)
	{
		return true;
	}

	return
		locomotionMode == LocomotionMode::Idle ||
		locomotionMode == LocomotionMode::Walk ||
		locomotionMode == LocomotionMode::Run;
}

bool ResolveAbilityStateSystem::IsCancelRuleActive(
	const AbilityTransitionRuleDef& cancelRule,
	const AbilityDef& currentAbilityDef,
	const AbilityStateComp& abilityState,
	CharacterId characterId,
	const ActorAbilityInputEvent& input,
	bool fromBufferedInput,
	const PlayerNetworkCompensationComp* networkCompensation)
{
	if (cancelRule.windowPolicy == AbilityTransitionWindowPolicy::Always)
	{
		return true;
	}

	const float serverProgress =
		(currentAbilityDef.timeline.durationSec > 0.0f)
		? ClampFloat(abilityState.elapsedSec / currentAbilityDef.timeline.durationSec, 0.0f, 1.0f)
		: 1.0f;
	const float windowStart = cancelRule.windowStartNormalized.value_or(0.0f);
	const float windowEnd = cancelRule.windowEndNormalized.value_or(1.0f);

	if (ShouldUseClientAnimationTiming(cancelRule.cause) &&
		!fromBufferedInput &&
		input.hasClientAnimationTiming)
	{
		const AnimationId expectedAnimId =
			ResolveAbilityAnimationId(characterId, abilityState.abilityId);
		const float clientComboTimingDriftTolerance =
			ResolveClientComboTimingDriftTolerance(networkCompensation);
		const float clientProgress = ClampFloat(
			input.clientNormalizedTime,
			0.0f,
			1.0f);
		const bool instanceMatches =
			input.clientAbilityInstanceId == abilityState.abilityInstanceId;
		const bool animMatches =
			expectedAnimId == AnimationId::None ||
			input.clientAnimId == expectedAnimId;
		const bool clientWindowMatches =
			clientProgress >= windowStart && clientProgress <= windowEnd;
		const bool driftMatches =
			std::abs(clientProgress - serverProgress) <=
			clientComboTimingDriftTolerance;
		const bool accepted =
			instanceMatches &&
			animMatches &&
			clientWindowMatches &&
			driftMatches;

		return accepted;
	}

	if (ShouldUseClientAnimationTiming(cancelRule.cause))
	{
		const bool accepted =
			serverProgress >= windowStart && serverProgress <= windowEnd;
		return accepted;
	}

	return serverProgress >= windowStart && serverProgress <= windowEnd;
}

bool ResolveAbilityStateSystem::ShouldUseClientAnimationTiming(
	AbilityTransitionCause cause) noexcept
{
	return
		cause == AbilityTransitionCause::Combo ||
		cause == AbilityTransitionCause::LightAttackCancel ||
		cause == AbilityTransitionCause::HeavyAttackCancel;
}

float ResolveAbilityStateSystem::ResolveAbilityInputBufferDurationSec(
	const PlayerNetworkCompensationComp* networkCompensation) noexcept
{
	return networkCompensation != nullptr
		? networkCompensation->abilityInputBufferDurationSec
		: kDefaultAbilityInputBufferDurationSec;
}

float ResolveAbilityStateSystem::ResolveClientComboTimingDriftTolerance(
	const PlayerNetworkCompensationComp* networkCompensation) noexcept
{
	return networkCompensation != nullptr
		? networkCompensation->attackDriftTolerance01
		: kMaxClientComboTimingDriftTolerance;
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
	const float nextElapsedSec =
		ComputeAdvancedElapsedSec(abilityState, abilityDef, deltaTimeSec);

	advance.abilityId = abilityState.abilityId;
	advance.abilityInstanceId = abilityState.abilityInstanceId;
	advance.prevElapsedSec = abilityState.elapsedSec;
	advance.currElapsedSec = nextElapsedSec;
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
