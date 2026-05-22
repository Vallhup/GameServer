#pragma once

#include "System.h"
#include "SystemMetaStorage.h"
#include "../../GameplayRuntimeComponents.h"
#include "../AbilityProfileService.h"

class ResolveAbilityStateSystem final : public System
{
public:
    void Execute(SystemContext& ctx) override;
    const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	struct RequestCandidate
	{
		AbilityId abilityId{ InvalidAbilityId };
		Entity target{ Entity::Null() };
		float directionX{ 0.0f };
		float directionZ{ 0.0f };
		bool useInputDirection{ false };
		ActorAbilityInputEvent request;
		bool fromBufferedInput{ false };
	};

	struct TransitionDecision
	{
		AbilityId nextAbilityId{ InvalidAbilityId };
		bool transition{ false };
		bool consumeOnRequestCosts{ false };
		bool preserveDirection{ false };
		bool consumeAbilityInput{ false };
		Entity target{ Entity::Null() };
		float directionX{ 0.0f };
		float directionZ{ 0.0f };
	};

    static const StaticSystemMetaStorage<15, 0, 2> kMetaStorage;

	static bool TryHandleBlockingState(
		SystemContext& ctx,
		Entity entity,
		AbilityStateComp& abilityState,
		ActorInputComp& input,
		AbilityTimelineAdvanceComp& advance);

	static bool TryResolveInterruptTransition(
		const AbilityProfileService& profileService,
		const CharacterId characterId,
		const AbilityStateComp& abilityState,
		const AbilityDef* currentAbilityDef,
		const AbilityInterruptQueueComp* interruptQueue,
		TransitionDecision& outDecision);

	static bool TryResolveCancelTransition(
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
		TransitionDecision& outDecision);

	static bool TryResolveIdleRequestTransition(
		const AbilityProfileService& profileService,
		const CharacterId characterId,
		const LocomotionStateComp& locomotionState,
		const WorldTransformComp& transform,
		const ActorInputComp& input,
		const CombatStatStateComp* stats,
		const ConsumableInventoryComp* inventory,
		const AIPerceptionComp* perception,
		const GameplayTagStateComp* tags,
		TransitionDecision& outDecision);

	static bool TryResolveEndPolicyTransition(
		const AbilityStateComp& abilityState,
		const AbilityDef& abilityDef,
		const ActorInputComp& input,
		AbilityTimelineAdvanceComp& advance,
		double deltaTimeSec,
		TransitionDecision& outDecision);

	static void ApplyTransition(
		AbilityStateComp& abilityState,
		const TransitionDecision& decision);

	static void ConsumeOnRequestResourceCosts(
		SystemContext& ctx,
		Entity entity,
		const TransitionDecision& decision);

	static void UpdateAbilityInputBuffer(
		ActorInputComp& input,
		double deltaTimeSec);

	static bool HasAbilityInputRequest(
		const ActorAbilityInputEvent& input) noexcept;

	static bool CanBufferAbilityInput(
		const ActorAbilityInputEvent& input) noexcept;

	static void BufferCurrentAbilityInput(
		ActorInputComp& input);

	static void ClearAbilityInput(ActorInputComp& input);

	static void ClearAbilityInputBuffer(ActorInputComp& input);

	static void ClearAllAbilityInput(ActorInputComp& input);

	static void FinishAbilityInput(
		ActorInputComp& input,
		bool consumed,
		bool allowBuffering);

	static std::vector<RequestCandidate> BuildRequestCandidates(
		const AbilityProfileService& profileService,
		CharacterId characterId,
		const ActorInputComp& input,
		bool includeHeldGuardRequest);

	static bool IsAbilityRequestAllowed(
		AbilityId abilityId,
		const CombatStatStateComp* stats,
		const ConsumableInventoryComp* inventory,
		const AIPerceptionComp* perception,
		const GameplayTagStateComp* tags);

	static bool IsAbilityStartLocomotionAllowed(
		AbilityId abilityId,
		LocomotionMode locomotionMode);

	static bool IsCancelRuleActive(
		const AbilityTransitionRuleDef& cancelRule,
		const AbilityDef& currentAbilityDef,
		const AbilityStateComp& abilityState,
		CharacterId characterId,
		const ActorAbilityInputEvent& input,
		bool fromBufferedInput);

	static bool ShouldUseClientAnimationTiming(
		AbilityTransitionCause cause) noexcept;

	static bool IsHoldReleased(
		const AbilityDef& abilityDef,
		const ActorInputComp& input);

	static float ComputeAdvancedElapsedSec(
		const AbilityStateComp& abilityState,
		const AbilityDef& abilityDef,
		double deltaTimeSec);

	static void PrepareTimelineAdvance(
		const AbilityStateComp& abilityState,
		const AbilityDef& abilityDef,
		AbilityTimelineAdvanceComp& advance,
		double deltaTimeSec);

	static void PrepareStartedAbilityAdvance(
		const AbilityStateComp& abilityState,
		AbilityTimelineAdvanceComp& advance);

	static void SetAbilityDirectionOnStart(
		AbilityStateComp& abilityState,
		const TransitionDecision& decision,
		const LocomotionStateComp& locomotionState,
		const WorldTransformComp& transform);

	static void ResetAbilityDirection(AbilityStateComp& abilityState);
};
