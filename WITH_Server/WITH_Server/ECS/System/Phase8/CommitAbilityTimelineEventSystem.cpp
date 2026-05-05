#include "pch.h"
#include "CommitAbilityTimelineEventSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 6> kCommitAbilityTimelineEventAccesses{
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		ReadImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteImmediate(ComponentRes<PendingAbilityPresentationEventComp>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
		WriteDeferred(ComponentRes<PendingGameplayEffectApplyComp>()),
	};
}

const SystemMeta CommitAbilityTimelineEventSystem::kMeta =
	SystemMeta{
		SysTag<CommitAbilityTimelineEventSystem>(),
		"CommitAbilityTimelineEventSystem",
		kCommitAbilityTimelineEventAccesses,
		kNoDeps,
		kNoDeps
	};

void CommitAbilityTimelineEventSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, advance] :
		ctx.ecs.View<AbilityTimelineAdvanceComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			continue;
		}

		for (const AbilityEventDef& eventDef : advance.events)
		{
			if (eventDef.condition == AbilityEventTriggerCondition::OnParrySuccess)
			{
				const PendingCombatResultComp* result =
					ctx.ecs.GetComponent<PendingCombatResultComp>(entity);
				if (result == nullptr || !result->parrySucceededThisFrame)
				{
					continue;
				}
			}

			if (eventDef.kind == AbilityEventKind::SpawnProjectile)
			{
				PendingProjectileSpawnComp* projectile =
					ctx.ecs.GetMutableComponent<PendingProjectileSpawnComp>(entity);
				if (projectile == nullptr)
				{
					continue;
				}

				projectile->requests.push_back(PendingProjectileSpawnRequest{
					.sourceEntity = entity,
					.sourceAbilityId = advance.abilityId,
					.sourceAbilityInstanceId = advance.abilityInstanceId,
					.payloadId = eventDef.payloadId
				});
			}
			else if (eventDef.kind == AbilityEventKind::ApplyGameplayEffect &&
				eventDef.effectId.has_value())
			{
				ctx.runtime.DeferredUpsertComponent<PendingGameplayEffectApplyComp>(
					entity,
					PendingGameplayEffectApplyComp{
						.effectId = *eventDef.effectId
					});
			}
			else if (eventDef.kind == AbilityEventKind::PlayCue)
			{
				PendingAbilityPresentationEventComp* presentation =
					ctx.ecs.GetMutableComponent<PendingAbilityPresentationEventComp>(entity);
				if (presentation == nullptr)
				{
					continue;
				}

				presentation->events.push_back(PendingAbilityPresentationEvent{
					.sourceEntity = entity,
					.sourceAbilityId = advance.abilityId,
					.sourceAbilityInstanceId = advance.abilityInstanceId,
					.eventKind = eventDef.kind,
					.payloadId = eventDef.payloadId
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

const SystemMeta& CommitAbilityTimelineEventSystem::Meta() const
{
	return kMeta;
}
