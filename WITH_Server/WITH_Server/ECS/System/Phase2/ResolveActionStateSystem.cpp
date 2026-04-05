#include "pch.h"
#include "ResolveActionStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	constexpr float kHoldActionElapsedEpsilonSec = 1.0e-4f;

	void ResetActionDirection(ActionStateComp& actionState)
	{
		actionState.directionX = 0.0f;
		actionState.directionZ = 0.0f;
	}

	void SetActionDirectionFromInputOrFacing(
		ActionStateComp& actionState,
		const PlayerActionInputEvent& actionInput,
		const LocomotionStateComp& locomotionState,
		const WorldTransformComp& transform)
	{
		float dirX = actionInput.directionX;
		float dirZ = actionInput.directionZ;
		NormalizeXZ(dirX, dirZ);

		if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
		{
			dirX = locomotionState.desiredMoveDirX;
			dirZ = locomotionState.desiredMoveDirZ;
			NormalizeXZ(dirX, dirZ);
		}

		if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
		{
			dirX = -std::sin(transform.yawRad);
			dirZ = -std::cos(transform.yawRad);
			NormalizeXZ(dirX, dirZ);
		}

		actionState.directionX = dirX;
		actionState.directionZ = dirZ;
	}

	bool IsHoldReleased(
		const ActionDef& actionDef,
		const PlayerInputComp& input)
	{
		return
			actionDef.normalizedPolicy == ActionNormalizedPolicy::Holdable &&
			actionDef.endPolicy.endType == ActionEndType::HoldRelease &&
			actionDef.kind == ActionKind::Guard &&
			!input.guard.isPressed;
	}

	void ClampHoldableElapsedSec(ActionStateComp& actionState, const ActionDef& actionDef)
	{
		if (actionDef.normalizedPolicy != ActionNormalizedPolicy::Holdable)
		{
			return;
		}

		const float holdElapsedSec =
			std::max(
				0.0f,
				actionDef.duration - kHoldActionElapsedEpsilonSec);
		actionState.elapsedSec = std::min(actionState.elapsedSec, holdElapsedSec);
	}

	ActionId FindGuardAction(CharacterId characterId)
	{
		for (const ActionDef& def : GetActionDefs())
		{
			if (def.characterId == characterId &&
				def.kind == ActionKind::Guard &&
				def.playerInput == PlayerActionInput::Guard)
			{
				return def.id;
			}
		}

		return ActionId::None;
	}
}

const SystemMeta ResolveActionStateSystem::kMeta =
	MakeSystemMeta<ResolveActionStateSystem>("ResolveActionStateSystem");

void ResolveActionStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, actionState, locomotionState, transform, input, advance] :
		ctx.ecs.View<
			ActionStateComp,
			LocomotionStateComp,
			WorldTransformComp,
			PlayerInputComp,
			ActionTimelineAdvanceComp>())
	{
		ClearActionTimelineAdvance(advance);

		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			actionState.actionId = ActionId::None;
			actionState.elapsedSec = 0.0f;
			ResetActionDirection(actionState);
			input.action = {};
			continue;
		}

		if (const auto* pending =
			ctx.ecs.GetComponent<PendingKnockdownComp>(entity))
		{
			if (pending->payload.reactionActionId != ActionId::None)
			{
				++actionState.actionInstanceId;
				actionState.actionId = pending->payload.reactionActionId;
				actionState.elapsedSec = 0.0f;
				ResetActionDirection(actionState);
			}
			ctx.runtime.DeferredRemoveComponent<PendingKnockdownComp>(entity);
			input.action = {};
			continue;
		}

		if (const auto* pending =
			ctx.ecs.GetComponent<PendingGuardBreakComp>(entity))
		{
			if (pending->payload.reactionActionId != ActionId::None)
			{
				++actionState.actionInstanceId;
				actionState.actionId = pending->payload.reactionActionId;
				actionState.elapsedSec = 0.0f;
				ResetActionDirection(actionState);
			}
			ctx.runtime.DeferredRemoveComponent<PendingGuardBreakComp>(entity);
			input.action = {};
			continue;
		}

		if (const auto* pending =
			ctx.ecs.GetComponent<PendingHitReactionComp>(entity))
		{
			if (pending->payload.reactionActionId != ActionId::None)
			{
				++actionState.actionInstanceId;
				actionState.actionId = pending->payload.reactionActionId;
				actionState.elapsedSec = 0.0f;
				ResetActionDirection(actionState);
			}
			ctx.runtime.DeferredRemoveComponent<PendingHitReactionComp>(entity);
			input.action = {};
			continue;
		}

		if (IsActionActive(actionState))
		{
			const ActionDef* actionDef = FindActionDef(actionState.actionId);
			if (actionDef == nullptr)
			{
				actionState = {};
				input.action = {};
				continue;
			}

			advance.actionId = actionState.actionId;
			advance.actionInstanceId = actionState.actionInstanceId;
			advance.prevElapsedSec = actionState.elapsedSec;

			actionState.elapsedSec += static_cast<float>(ctx.dtSec);
			ClampHoldableElapsedSec(actionState, *actionDef);
			advance.currElapsedSec = actionState.elapsedSec;

			const float duration = std::max(0.001f, actionDef->duration);
			for (const ActionEventDef& eventDef : actionDef->events)
			{
				const float eventTimeSec = eventDef.timeNormalized * duration;
				if (eventTimeSec < advance.prevElapsedSec ||
					eventTimeSec >= advance.currElapsedSec)
				{
					continue;
				}

				advance.events.push_back(PendingActionTimelineEvent{
					eventDef.type,
					eventDef.timeNormalized,
					eventDef.payloadId,
					eventDef.conditionType,
					actionState.actionId,
					actionState.actionInstanceId
				});
			}

			if ((actionState.elapsedSec >= actionDef->duration &&
				actionDef->normalizedPolicy == ActionNormalizedPolicy::FixedDuration) ||
				IsHoldReleased(*actionDef, input))
			{
				actionState.actionId = actionDef->endPolicy.defaultNextActionId;
				actionState.elapsedSec = 0.0f;
				ResetActionDirection(actionState);
			}

			input.action = {};
			continue;
		}

		const SpawnTypeComp* spawnType =
			ctx.ecs.GetComponent<SpawnTypeComp>(entity);

		if (input.guard.isPressed &&
			input.action.type == PlayerActionInputType::None &&
			spawnType != nullptr)
		{
			const ActionId guardActionId =
				FindGuardAction(spawnType->characterId);
			if (guardActionId != ActionId::None)
			{
				++actionState.actionInstanceId;
				actionState.actionId = guardActionId;
				actionState.elapsedSec = 0.0f;
				SetActionDirectionFromInputOrFacing(
					actionState,
					input.action,
					locomotionState,
					transform);
			}
		}

		if (input.action.type == PlayerActionInputType::None)
		{
			continue;
		}

		if (spawnType != nullptr)
		{
			const ActionId actionId = FindActionForInput(
				spawnType->characterId,
				input.action.type);
			if (actionId != ActionId::None)
			{
				++actionState.actionInstanceId;
				actionState.actionId = actionId;
				actionState.elapsedSec = 0.0f;
				SetActionDirectionFromInputOrFacing(
					actionState,
					input.action,
					locomotionState,
					transform);
			}
		}

		input.action = {};
	}
}

const SystemMeta& ResolveActionStateSystem::Meta() const
{
	return kMeta;
}
