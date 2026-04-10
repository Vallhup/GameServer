#pragma once

#include "System.h"
#include "SystemMetaStorage.h"
#include "../../GameplayRuntimeComponents.h"
#include "../ActionProfileService.h"

class ResolveActionStateSystem final : public System
{
public:
    void Execute(SystemContext& ctx) override;
    const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	struct RequestCandidate
	{
		ActionId actionId{ ActionId::None };
		float directionX{ 0.0f };
		float directionZ{ 0.0f };
		bool useInputDirection{ false };
	};

	struct TransitionDecision
	{
		ActionId nextActionId{ ActionId::None };
		bool transition{ false };
		bool consumeOnRequestCosts{ false };
		bool preserveDirection{ false };
		float directionX{ 0.0f };
		float directionZ{ 0.0f };
	};

    static const StaticSystemMetaStorage<12, 0, 2> kMetaStorage;

	static bool TryHandleBlockingState(
		SystemContext& ctx,
		Entity entity,
		ActionStateComp& actionState,
		ActorInputComp& input,
		ActionTimelineAdvanceComp& advance);

	static bool TryResolveInterruptTransition(
		const ActionProfileService& profileService,
		const CharacterId characterId,
		const ActionStateComp& actionState,
		const ActionDef* currentActionDef,
		const ActionInterruptQueueComp* interruptQueue,
		TransitionDecision& outDecision);

	static bool TryResolveCancelTransition(
		const ActionProfileService& profileService,
		const CharacterId characterId,
		const ActionStateComp& actionState,
		const ActionDef& currentActionDef,
		const LocomotionStateComp& locomotionState,
		const WorldTransformComp& transform,
		const ActorInputComp& input,
		const CombatStatStateComp* stats,
		const AIPerceptionComp* perception,
		TransitionDecision& outDecision);

	static bool TryResolveIdleRequestTransition(
		const ActionProfileService& profileService,
		const CharacterId characterId,
		const LocomotionStateComp& locomotionState,
		const WorldTransformComp& transform,
		const ActorInputComp& input,
		const CombatStatStateComp* stats,
		const AIPerceptionComp* perception,
		TransitionDecision& outDecision);

	static bool TryResolveEndPolicyTransition(
		const ActionStateComp& actionState,
		const ActionDef& actionDef,
		const ActorInputComp& input,
		ActionTimelineAdvanceComp& advance,
		double deltaTimeSec,
		TransitionDecision& outDecision);

	static void ApplyTransition(
		ActionStateComp& actionState,
		const TransitionDecision& decision);

	static void ConsumeOnRequestResourceCosts(
		SystemContext& ctx,
		Entity entity,
		const TransitionDecision& decision);

	static void ClearActionInput(ActorInputComp& input);

	static std::vector<RequestCandidate> BuildRequestCandidates(
		const ActionProfileService& profileService,
		CharacterId characterId,
		const ActorInputComp& input,
		bool includeHeldGuardRequest);

	static bool IsActionRequestAllowed(
		ActionId actionId,
		const CombatStatStateComp* stats,
		const AIPerceptionComp* perception);

	static bool IsCancelRuleActive(
		const ActionCancelRule& cancelRule,
		const ActionDef& currentActionDef,
		const ActionStateComp& actionState);

	static bool IsHoldReleased(
		const ActionDef& actionDef,
		const ActorInputComp& input);

	static float ComputeAdvancedElapsedSec(
		const ActionStateComp& actionState,
		const ActionDef& actionDef,
		double deltaTimeSec);

	static void PrepareTimelineAdvance(
		const ActionStateComp& actionState,
		const ActionDef& actionDef,
		ActionTimelineAdvanceComp& advance,
		double deltaTimeSec);

	static void PrepareStartedActionAdvance(
		const ActionStateComp& actionState,
		ActionTimelineAdvanceComp& advance);

	static void SetActionDirectionOnStart(
		ActionStateComp& actionState,
		const TransitionDecision& decision,
		const LocomotionStateComp& locomotionState,
		const WorldTransformComp& transform);

	static void ResetActionDirection(ActionStateComp& actionState);
};
