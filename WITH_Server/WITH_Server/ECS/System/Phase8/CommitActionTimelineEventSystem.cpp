#include "pch.h"
#include "CommitActionTimelineEventSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 5> kCommitActionTimelineEventAccesses{
		ReadSnapshot(ComponentRes<ActionTimelineAdvanceComp>()),
		ReadSnapshot(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteImmediate(ComponentRes<PendingActionPresentationEventComp>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
	};
}

const SystemMeta CommitActionTimelineEventSystem::kMeta =
	SystemMeta{
		SysTag<CommitActionTimelineEventSystem>(),
		"CommitActionTimelineEventSystem",
		kCommitActionTimelineEventAccesses,
		kNoDeps,
		kNoDeps
	};

void CommitActionTimelineEventSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, advance] :
		ctx.ecs.View<ActionTimelineAdvanceComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			continue;
		}

		for (const PendingActionTimelineEvent& eventRecord : advance.events)
		{
			if (eventRecord.conditionType == TriggerConditionType::OnParrySuccess)
			{
				const PendingCombatResultComp* result =
					ctx.ecs.GetComponent<PendingCombatResultComp>(entity);
				if (result == nullptr || !result->parrySucceededThisFrame)
				{
					continue;
				}
			}

			if (eventRecord.eventType == EventType::SpawnProjectile)
			{
				PendingProjectileSpawnComp* projectile =
					ctx.ecs.GetMutableComponent<PendingProjectileSpawnComp>(entity);
				if (projectile == nullptr)
				{
					continue;
				}

				projectile->requests.push_back(PendingProjectileSpawnRequest{
					entity,
					eventRecord.sourceActionId,
					eventRecord.sourceActionInstanceId,
					eventRecord.payloadId
				});
			}
			else if (eventRecord.eventType == EventType::PlayEffect)
			{
				PendingActionPresentationEventComp* presentation =
					ctx.ecs.GetMutableComponent<PendingActionPresentationEventComp>(entity);
				if (presentation == nullptr)
				{
					continue;
				}

				presentation->events.push_back(PendingActionPresentationEvent{
					entity,
					eventRecord.sourceActionId,
					eventRecord.sourceActionInstanceId,
					eventRecord.eventType,
					eventRecord.payloadId
				});
			}
			else
			{
				ReplicationStatsComp* stats =
					ctx.ecs.GetMutableComponent<ReplicationStatsComp>(entity);
				if (stats != nullptr)
				{
					++stats->deferredPotionEventCount;
				}
			}
		}
	}
}

const SystemMeta& CommitActionTimelineEventSystem::Meta() const
{
	return kMeta;
}
