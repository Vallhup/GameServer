#include "pch.h"
#include "CommitAbilityTimelineEventSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	bool TryConsumeHpPotion(SystemContext& ctx, Entity entity)
	{
		ConsumableInventoryComp* inventory =
			ctx.ecs.GetMutableComponent<ConsumableInventoryComp>(entity);
		CombatStatStateComp* stats =
			ctx.ecs.GetMutableComponent<CombatStatStateComp>(entity);
		if (inventory == nullptr ||
			stats == nullptr ||
			inventory->hpPotionCount == 0 ||
			stats->currentHp <= 0 ||
			stats->maxHp <= 0 ||
			stats->currentHp >= stats->maxHp)
		{
			return false;
		}

		const HpPotionTuning tuning{};
		--inventory->hpPotionCount;
		if (DirtyFlagsComp* dirty =
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
		{
			dirty->MarkDirty(WorldDirtyType::Inventory);
		}

		const int32_t previousHp = stats->currentHp;
		stats->currentHp = std::clamp(
			stats->currentHp + tuning.healAmount,
			0,
			stats->maxHp);

		if (previousHp != stats->currentHp)
		{
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::Stat);
			}
		}

		return true;
	}
}

const StaticSystemMetaStorage<9> CommitAbilityTimelineEventSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<CommitAbilityTimelineEventSystem>(),
		"CommitAbilityTimelineEventSystem",
		std::array<AccessSpec, 9>
	{
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		ReadImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteImmediate(ComponentRes<PendingAbilityPresentationEventComp>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
		WriteImmediate(ComponentRes<PendingGameplayEffectApplyComp>()),
		WriteImmediate(ComponentRes<ConsumableInventoryComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
	});

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
				if (PendingGameplayEffectApplyComp* effectApply =
					ctx.ecs.GetMutableComponent<PendingGameplayEffectApplyComp>(
						entity))
				{
					effectApply->effectId = *eventDef.effectId;
				}
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
			else if (eventDef.kind == AbilityEventKind::ConsumeItem)
			{
				if (eventDef.payloadId.has_value() &&
					*eventDef.payloadId ==
						static_cast<uint16_t>(ConsumableItemId::HpPotion))
				{
					(void)TryConsumeHpPotion(ctx, entity);
				}
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
