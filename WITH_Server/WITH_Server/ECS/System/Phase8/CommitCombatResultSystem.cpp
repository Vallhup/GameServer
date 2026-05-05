#include "pch.h"
#include "CommitCombatResultSystem.h"

#include "../../../AIBehaviorDef.h"
#include "../../../GameDataCatalog.h"
#include "../GameplaySystemUtil.h"
#include "RepComponent.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 12> kCommitCombatResultAccesses{
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		WriteImmediate(ComponentRes<AIReactionComp>()),
		WriteImmediate(ComponentRes<BossPhaseStateComp>()),
		WriteImmediate(ComponentRes<BossPatternRuntimeComp>()),
		WriteImmediate(ComponentRes<AbilityInterruptQueueComp>()),
		ReadImmediate(ComponentRes<AITypeComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteDeferred(CommandBufferRes()),
		WriteDeferred(ComponentRes<PendingGameplayEffectApplyComp>()),
	};
}

const SystemMeta CommitCombatResultSystem::kMeta =
	SystemMeta{
		SysTag<CommitCombatResultSystem>(),
		"CommitCombatResultSystem",
		kCommitCombatResultAccesses,
		kNoDeps,
		kNoDeps,
		true,
		false
	};

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
				return lhs.sourceAbilityInstanceId <
					rhs.sourceAbilityInstanceId;
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
					interaction.parryEffect->grantEffectId.has_value() &&
					!result.pendingParryEffectId.has_value())
				{
					result.pendingParryEffectId =
						interaction.parryEffect->grantEffectId;
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

		// Boss phase threshold reaction event.
		if (stats.maxHp > 0 && previousStats.currentHp > stats.currentHp)
		{
			BossPhaseStateComp* bossPhase =
				ctx.ecs.GetMutableComponent<BossPhaseStateComp>(entity);
			const AITypeComp* aiType = ctx.ecs.GetComponent<AITypeComp>(entity);
			const AIBehaviorProfileDef* profile =
				aiType != nullptr
					? GameDataCatalog::Current().AIBehaviors().Find(
						aiType->aiProfileId)
					: nullptr;
			if (bossPhase != nullptr && profile != nullptr)
			{
				const float previousHpRatio =
					static_cast<float>(previousStats.currentHp) /
					static_cast<float>(stats.maxHp);
				const float currentHpRatio =
					static_cast<float>(stats.currentHp) /
					static_cast<float>(stats.maxHp);

				for (size_t transitionIndex = 0;
					transitionIndex < profile->bossPattern.phaseTransitions.size();
					++transitionIndex)
				{
					const AIBossPhaseTransitionDef& transition =
						profile->bossPattern.phaseTransitions[transitionIndex];
					const uint32_t thresholdMask =
						1u << static_cast<uint32_t>(transitionIndex);
					if ((bossPhase->crossedThresholdMask & thresholdMask) != 0 ||
						previousHpRatio <= transition.hpRatio ||
						currentHpRatio > transition.hpRatio)
					{
						continue;
					}

					bossPhase->crossedThresholdMask |= thresholdMask;
					bossPhase->currentPhase = transition.phase;
					bossPhase->transitionRequested = true;

					if (BossPatternRuntimeComp* bossRuntime =
						ctx.ecs.GetMutableComponent<BossPatternRuntimeComp>(entity))
					{
						bossRuntime->phaseTransitionLockSec =
							transition.transitionLockSec;
						bossRuntime->phaseTransitionActionPending = true;
						bossRuntime->strafeTimeLeftSec = 0.0f;
					}

					if (auto* aiReaction =
						ctx.ecs.GetMutableComponent<AIReactionComp>(entity))
					{
						aiReaction->PostEvent(AIReactionEvent{
							.type = AIReactionEventType::OnHpThreshold,
							.instigator = result.reactionSource,
							.priority = 300,
							.floatPayload = transition.hpRatio,
						});
					}

					break;
				}
			}
		}

		// AI hit reaction event.
		if (result.wasHitThisFrame)
		{
			if (auto* aiReaction = ctx.ecs.GetMutableComponent<AIReactionComp>(entity))
			{
				aiReaction->PostEvent(AIReactionEvent{
					.type       = AIReactionEventType::OnHitReceived,
					.instigator = result.reactionSource,
					.priority   = 100,
				});
			}
		}

		// AI 패리 당함 반응 이벤트 등록 (공격자에게)
		for (const PendingCombatInteractionRecord& interaction :
			result.receivedInteractions)
		{
			if (interaction.resultType != CombatResolveResultType::Parry)
				continue;

			if (auto* aiReaction = ctx.ecs.GetMutableComponent<AIReactionComp>(interaction.sourceEntity))
			{
				aiReaction->PostEvent(AIReactionEvent{
					.type       = AIReactionEventType::OnParried,
					.instigator = entity,
					.priority   = 200,
				});
			}
		}

		const uint64_t frameIndex = ctx.runtime.FrameIndex();
		for (Entity parriedAttacker : parriedAttackers)
		{
			if (HasBlockingPendingState(ctx.ecs, parriedAttacker))
			{
				continue;
			}

			AbilityInterruptQueueComp* parriedInterruptQueue =
				ctx.ecs.GetMutableComponent<AbilityInterruptQueueComp>(
					parriedAttacker);
			if (parriedInterruptQueue == nullptr)
			{
				continue;
			}

			parriedInterruptQueue->events.push_back(AbilityInterruptEvent{
				.cause = AbilityTransitionCause::OnParried,
				.instigator = entity,
				.frameIndex = frameIndex,
				.priority = 500
			});
		}

		AbilityInterruptQueueComp* interruptQueue =
			ctx.ecs.GetMutableComponent<AbilityInterruptQueueComp>(entity);
		if (interruptQueue == nullptr)
		{
			continue;
		}

		if (stats.currentHp <= 0)
		{
			interruptQueue->events.push_back(AbilityInterruptEvent{
				.cause = AbilityTransitionCause::OnAttributeZero,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 1000
			});
		}
		else if (stats.currentPoise <= 0)
		{
			interruptQueue->events.push_back(AbilityInterruptEvent{
				.cause = AbilityTransitionCause::OnParried,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 300
			});
		}
		else if (result.reactionKind == CombatReactionKind::GuardBreak)
		{
			interruptQueue->events.push_back(AbilityInterruptEvent{
				.cause = AbilityTransitionCause::OnParried,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 200
			});
		}
		else if (result.reactionKind == CombatReactionKind::HitReaction)
		{
			interruptQueue->events.push_back(AbilityInterruptEvent{
				.cause = AbilityTransitionCause::OnHitReceived,
				.instigator = result.reactionSource,
				.frameIndex = frameIndex,
				.priority = 100
			});
		}

		if (result.pendingParryEffectId.has_value())
		{
			PendingGameplayEffectApplyComp effectApplyComp =
			{
				.effectId = *result.pendingParryEffectId
			};
			ctx.runtime.DeferredUpsertComponent<PendingGameplayEffectApplyComp>(
				entity, effectApplyComp);
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
