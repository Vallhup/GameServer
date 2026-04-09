#include "pch.h"
#include "AdvanceActionTimelineSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta AdvanceActionTimelineSystem::kMeta =
	MakeSystemMeta<AdvanceActionTimelineSystem>(
		"AdvanceActionTimelineSystem");

void AdvanceActionTimelineSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, actionState, advance] :
		ctx.ecs.View<ActionStateComp, ActionTimelineAdvanceComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			!IsActionActive(actionState))
		{
			continue;
		}

		const ActionDef* actionDef = FindActionDef(actionState.actionId);
		if (actionDef == nullptr)
		{
			continue;
		}

		const bool hasMatchingAdvance =
			advance.actionId == actionState.actionId &&
			advance.actionInstanceId == actionState.actionInstanceId;
		if (!hasMatchingAdvance)
		{
			continue;
		}

		ApplyElapsedToActiveAction(actionState, advance);
		CollectTimelineEvents(*actionDef, advance);
	}
}

const SystemMeta& AdvanceActionTimelineSystem::Meta() const
{
	return kMeta;
}

void AdvanceActionTimelineSystem::ApplyElapsedToActiveAction(
	ActionStateComp& actionState,
	const ActionTimelineAdvanceComp& advance)
{
	actionState.elapsedSec = advance.currElapsedSec;
}

void AdvanceActionTimelineSystem::CollectTimelineEvents(
	const ActionDef& actionDef,
	ActionTimelineAdvanceComp& advance)
{
	const float duration = std::max(0.001f, actionDef.duration);
	for (const ActionEventDef& eventDef : actionDef.events)
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
			advance.actionId,
			advance.actionInstanceId
		});
	}
}
