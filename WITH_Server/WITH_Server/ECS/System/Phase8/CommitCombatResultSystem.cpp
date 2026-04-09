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
		bool guardResolved = false;
		std::vector<Entity> parriedAttackers;
		const CombatStatStateComp previousStats = stats;
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
				guardResolved = true;
				float chipDamage = static_cast<float>(damage);
				float staminaDamageScale = 1.0f;
				if (interaction.guardEffect.has_value())
				{
					chipDamage =
						chipDamage *
						interaction.guardEffect->chipDamageRatio *
						std::max(
							0.0f,
							1.0f - interaction.guardEffect->damageReductionRatio);
					staminaDamageScale =
						interaction.guardEffect->staminaDamageMultiplier;
				}

				const int32_t guardedHpDamage = static_cast<int32_t>(std::lround(
					chipDamage * 100.0f / std::max(1, 100 + stats.defense)));
				const int32_t guardedStaminaDamage = static_cast<int32_t>(std::lround(
					static_cast<float>(staminaDamage) * staminaDamageScale));
				hpDelta -= std::max(0, guardedHpDamage);
				staminaDelta -= std::max(0, guardedStaminaDamage);
			}
			else if (interaction.resultType == CombatResolveResultType::Parry)
			{
				if (std::find(
					parriedAttackers.begin(),
					parriedAttackers.end(),
					interaction.sourceEntity) == parriedAttackers.end())
				{
					parriedAttackers.push_back(interaction.sourceEntity);
				}

				if (interaction.parryEffect.has_value() &&
					interaction.parryEffect->grantBuffId.has_value() &&
					!result.pendingParryBuffId.has_value())
				{
					result.pendingParryBuffId =
						interaction.parryEffect->grantBuffId;
				}
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

		if (guardResolved &&
			stats.currentHp > 0 &&
			stats.currentStamina <= 0)
		{
			result.reactionKind = CombatReactionKind::GuardBreak;
		}

		if (DidStatsChange(previousStats, stats))
		{
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::Stat);
			}
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

		const uint64_t frameIndex = ctx.runtime.FrameIndex();
		for (Entity parriedAttacker : parriedAttackers)
		{
			if (HasBlockingPendingState(ctx.ecs, parriedAttacker))
			{
				continue;
			}

			ActionInterruptQueueComp* parriedInterruptQueue =
				ctx.ecs.GetMutableComponent<ActionInterruptQueueComp>(
					parriedAttacker);
			if (parriedInterruptQueue == nullptr)
			{
				continue;
			}

			parriedInterruptQueue->events.push_back(ActionInterruptEvent{
				.causeType = ActionInterruptCauseType::OnParried,
				.instigator = entity,
				.frameIndex = frameIndex,
				.priority = 500
			});
		}

		ActionInterruptQueueComp* interruptQueue =
			ctx.ecs.GetMutableComponent<ActionInterruptQueueComp>(entity);
		if (interruptQueue == nullptr)
		{
			continue;
		}

		if (stats.currentHp <= 0)
		{
			interruptQueue->events.push_back(ActionInterruptEvent{
				.causeType = ActionInterruptCauseType::OnHpZero,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 1000
			});
		}
		else if (stats.currentPoise <= 0)
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

bool CommitCombatResultSystem::DidStatsChange(
	const CombatStatStateComp& previousStats,
	const CombatStatStateComp& currentStats) noexcept
{
	return
		previousStats.currentHp != currentStats.currentHp ||
		previousStats.currentStamina != currentStats.currentStamina ||
		previousStats.currentPoise != currentStats.currentPoise;
}
