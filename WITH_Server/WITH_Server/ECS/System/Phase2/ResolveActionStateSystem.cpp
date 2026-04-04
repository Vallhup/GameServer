#include "pch.h"
#include "ResolveActionStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveActionStateSystem::kMeta =
	MakeSystemMeta<ResolveActionStateSystem>("ResolveActionStateSystem");

void ResolveActionStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, actionState, input, advance] :
		ctx.ecs.View<
			ActionStateComp,
			PlayerInputComp,
			ActionTimelineAdvanceComp>())
	{
		ClearActionTimelineAdvance(advance);

		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			actionState.actionId = ActionId::None;
			actionState.elapsedSec = 0.0f;
			actionState.directionX = 0.0f;
			actionState.directionZ = 0.0f;
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
				actionState.directionX = 0.0f;
				actionState.directionZ = 0.0f;
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
				actionState.directionX = 0.0f;
				actionState.directionZ = 0.0f;
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
				actionState.directionX = 0.0f;
				actionState.directionZ = 0.0f;
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

			if (actionState.elapsedSec >= actionDef->duration &&
				actionDef->normalizedPolicy == ActionNormalizedPolicy::FixedDuration)
			{
				actionState.actionId = actionDef->endPolicy.defaultNextActionId;
				actionState.elapsedSec = 0.0f;
				actionState.directionX = 0.0f;
				actionState.directionZ = 0.0f;
			}

			input.action = {};
			continue;
		}

		if (input.action.type == PlayerActionInputType::None)
		{
			continue;
		}

		const SpawnTypeComp* spawnType =
			ctx.ecs.GetComponent<SpawnTypeComp>(entity);
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
				actionState.directionX = input.action.directionX;
				actionState.directionZ = input.action.directionZ;
				NormalizeXZ(actionState.directionX, actionState.directionZ);
			}
		}

		input.action = {};
	}
}

const SystemMeta& ResolveActionStateSystem::Meta() const
{
	return kMeta;
}
