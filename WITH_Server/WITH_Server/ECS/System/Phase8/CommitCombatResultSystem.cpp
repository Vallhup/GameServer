#include "pch.h"
#include "CommitCombatResultSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta CommitCombatResultSystem::kMeta =
	MakeSystemMeta<CommitCombatResultSystem>("CommitCombatResultSystem");

void CommitCombatResultSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, result, stats] :
		ctx.ecs.View<PendingCombatResultComp, CombatStatStateComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			result = {};
			continue;
		}

		std::sort(
			result.receivedInteractions.begin(),
			result.receivedInteractions.end(),
			[](const auto& lhs, const auto& rhs)
			{
				if (lhs.sourceEntity.id != rhs.sourceEntity.id)
				{
					return lhs.sourceEntity.id < rhs.sourceEntity.id;
				}
				return lhs.sourceActionInstanceId <
					rhs.sourceActionInstanceId;
			});

		int32_t hpDelta = 0;
		int32_t staminaDelta = 0;
		int32_t poiseDelta = 0;
		for (const PendingCombatInteractionRecord& interaction :
			result.receivedInteractions)
		{
			const CombatStatStateComp* sourceStats =
				ctx.ecs.GetComponent<CombatStatStateComp>(
					interaction.sourceEntity);
			if (sourceStats == nullptr)
			{
				continue;
			}

			const int32_t damage = static_cast<int32_t>(std::lround(
				sourceStats->attackPower *
				interaction.attackEffect.damageScale +
				interaction.attackEffect.bonusDamage));
			const int32_t staminaDamage = static_cast<int32_t>(std::lround(
				sourceStats->attackPower *
				interaction.attackEffect.staminaDamageScale +
				interaction.attackEffect.bonusStaminaDamage));
			const int32_t poiseDamage = static_cast<int32_t>(std::lround(
				sourceStats->attackPower *
				interaction.attackEffect.poiseDamageScale +
				interaction.attackEffect.bonusPoiseDamage));

			if (interaction.resultType == CombatResolveResultType::Hit)
			{
				hpDelta -= std::max(
					0,
					damage * 100 / std::max(1, 100 + stats.defense));
				staminaDelta -= std::max(0, staminaDamage);
				poiseDelta -= std::max(0, poiseDamage);
			}
			else if (interaction.resultType == CombatResolveResultType::Guard)
			{
				staminaDelta -= std::max(0, staminaDamage);
			}
		}

		stats.currentHp = std::clamp(
			stats.currentHp + hpDelta,
			0,
			stats.maxHp);
		stats.currentStamina = std::clamp(
			stats.currentStamina + staminaDelta,
			0,
			stats.maxStamina);
		stats.currentPoise = std::clamp(
			stats.currentPoise + poiseDelta,
			0,
			stats.maxPoise);

		if (DirtyFlagsComp* dirty =
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
		{
			dirty->MarkDirty(WorldDirtyType::Stat);
		}

		// AI 피격 반응 기록
		if (result.wasHitThisFrame)
		{
			if (auto* aiReaction = ctx.ecs.GetMutableComponent<AIReactionComp>(entity))
			{
				aiReaction->gotHitThisFrame = true;
				aiReaction->instigator = result.reactionSource;
			}
		}

		// AI 패리 당함 반응 기록 (이 엔티티의 공격을 피격자가 패리한 경우)
		for (const PendingCombatInteractionRecord& interaction :
			result.receivedInteractions)
		{
			if (interaction.resultType != CombatResolveResultType::Parry)
			{
				continue;
			}
			if (auto* aiReaction = ctx.ecs.GetMutableComponent<AIReactionComp>(interaction.sourceEntity))
			{
				aiReaction->gotParriedThisFrame = true;
				aiReaction->instigator = entity;
			}
		}

		if (stats.currentHp <= 0)
		{
			continue;
		}

		const SpawnTypeComp* spawnType =
			ctx.ecs.GetComponent<SpawnTypeComp>(entity);
		if (spawnType == nullptr)
		{
			continue;
		}

		ActionInterruptQueueComp* interruptQueue =
			ctx.ecs.GetMutableComponent<ActionInterruptQueueComp>(entity);
		if (interruptQueue == nullptr)
		{
			continue;
		}

		const uint64_t frameIndex = ctx.runtime.FrameIndex();
		if (stats.currentPoise <= 0)
		{
			interruptQueue->events.push_back(ActionInterruptEvent{
				.causeType = ActionInterruptCauseType::OnParried,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 300
			});
		}
		else if (result.reactionKind == CombatReactionKind::GuardBreak)
		{
			interruptQueue->events.push_back(ActionInterruptEvent{
				.causeType = ActionInterruptCauseType::OnParried,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 200
			});
		}
		else if (result.reactionKind == CombatReactionKind::HitReaction)
		{
			interruptQueue->events.push_back(ActionInterruptEvent{
				.causeType = ActionInterruptCauseType::OnHitReceived,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 100
			});
		}

		if (result.pendingParryBuffId.has_value())
		{
			PendingBuffApplyComp buffApplyComp =
			{
				.buffId = *result.pendingParryBuffId
			};
			ctx.runtime.DeferredUpsertComponent<PendingBuffApplyComp>(
				entity, buffApplyComp);
		}
	}
}

const SystemMeta& CommitCombatResultSystem::Meta() const
{
	return kMeta;
}
