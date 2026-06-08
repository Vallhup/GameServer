#include "pch.h"
#include "CommitCombatResultSystem.h"

#include <random>

#include "../../../AIBehaviorDef.h"
#include "../../../CharacterDef.h"
#include "../../../CharacterIdPolicy.h"
#include "../../../GameDataCatalog.h"
#include "../../../GameplayContentCatalog.h"
#include "../GameplaySystemUtil.h"
#include "BossGimmickCombatPolicy.h"
#include "ECS/Components/GameplayInputComponents.h"
#include "ECS/Components/GameplayWorldLifecycleComponents.h"
#include "RepComponent.h"

using namespace GameplaySystemUtil;

namespace
{
	bool RollKillBuffGrant(float grantProbability) noexcept
	{
		thread_local std::mt19937 rng{ std::random_device{}() };
		thread_local std::uniform_real_distribution<float> dist{ 0.0f, 1.0f };
		return dist(rng) < grantProbability;
	}

	void ApplyStaminaRecoveryDelay(
		SystemContext& ctx,
		Entity entity,
		const CombatStatStateComp& stats,
		float delaySec)
	{
		StaminaRecoveryStateComp* recovery =
			ctx.ecs.GetMutableComponent<StaminaRecoveryStateComp>(entity);
		if (recovery == nullptr)
		{
			return;
		}

		if (stats.currentStamina <= 0)
		{
			delaySec = std::max(delaySec, recovery->tuning.exhaustedRegenDelaySec);
		}

		recovery->regenLockRemainingSec =
			std::max(recovery->regenLockRemainingSec, delaySec);
	}
}

const StaticSystemMetaStorage<21> CommitCombatResultSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<CommitCombatResultSystem>(),
		"CommitCombatResultSystem",
		std::array<AccessSpec, 21>
	{
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<StaminaRecoveryStateComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		WriteImmediate(ComponentRes<AIReactionEventQueueComp>()),
		WriteImmediate(ComponentRes<AIPhaseRuntimeComp>()),
		WriteImmediate(ComponentRes<BossGimmickStateComp>()),
		WriteImmediate(ComponentRes<AIActionRuntimeComp>()),
		WriteImmediate(ComponentRes<AIMovementRuntimeComp>()),
		WriteImmediate(ComponentRes<AIBlackboardComp>()),
		WriteImmediate(ComponentRes<AbilityInterruptQueueComp>()),
		ReadImmediate(ComponentRes<AITypeComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteImmediate(ComponentRes<PendingGameplayEffectApplyComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		WriteImmediate(ComponentRes<PendingKillBuffGrantComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		WriteImmediate(ComponentRes<PendingPlayerDeathCountEventComp>()),
		WriteImmediate(ComponentRes<PendingMonsterKillEventComp>()),
		WriteImmediate(ComponentRes<PendingFinalBossDefeatedEventComp>()),
	});

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
		Entity killerEntity = Entity::Null();
		const CombatStatStateComp previousStats = stats;
		const BossGimmickStateComp* bossGimmick =
			ctx.ecs.GetComponent<BossGimmickStateComp>(entity);
		const bool ignoreIncomingHpDamage =
			bossGimmick != nullptr &&
			BossGimmickCombatPolicy::ShouldIgnoreIncomingHpDamage(*bossGimmick);
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
				if (ignoreIncomingHpDamage)
				{
					continue;
				}

				const int32_t netHpDamage = std::max(
					0,
					damage * 100 / std::max(1, 100 + stats.defense));

				// 이번 타격으로 HP 가 0 이하가 되는 최초 타격 = 킬 블로우.
				if (killerEntity.IsNull() &&
					stats.currentHp > 0 &&
					stats.currentHp + hpDelta - netHpDamage <= 0)
				{
					killerEntity = interaction.sourceEntity;
				}

				hpDelta -= netHpDamage;
				staminaDelta -= std::max(0, staminaDamage);
				poiseDelta -= std::max(0, poiseDamage);
			}
			else if (interaction.resultType == CombatResolveResultType::Guard)
			{
				if (ignoreIncomingHpDamage)
				{
					continue;
				}

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

		const AITypeComp* finalGimmickAIType =
			ctx.ecs.GetComponent<AITypeComp>(entity);
		const bool supportsFinalSafeZoneGimmick =
			finalGimmickAIType != nullptr &&
			finalGimmickAIType->aiType == AIArchetype::FinalBossMonster;
		if (supportsFinalSafeZoneGimmick &&
			previousStats.currentHp > 0 &&
			stats.currentHp <= 0)
		{
			if (BossGimmickStateComp* gimmick =
				ctx.ecs.GetMutableComponent<BossGimmickStateComp>(entity);
				gimmick != nullptr &&
				!gimmick->finalGimmickRequested &&
				!gimmick->finalGimmickCompleted)
			{
				stats.currentHp = 1;
				killerEntity = Entity::Null();
				gimmick->Request(BossGimmickType::FinalSafeZone);
			}
		}

		if (supportsFinalSafeZoneGimmick &&
			previousStats.currentHp > 0 &&
			stats.currentHp <= 0)
		{
			if (PendingFinalBossDefeatedEventComp* const finalBossDefeated =
				ctx.ecs.GetMutableComponent<PendingFinalBossDefeatedEventComp>(
					entity))
			{
				finalBossDefeated->pending = true;
			}
		}

		// 킬 버프 적재: 킬 블로우 판정 시 킬러가 EffectUser 라면 버프를 예약한다.
		if (!killerEntity.IsNull() && stats.currentHp <= 0)
		{
			PendingKillBuffGrantComp* killerBuffComp =
				ctx.ecs.GetMutableComponent<PendingKillBuffGrantComp>(killerEntity);
			if (killerBuffComp != nullptr)
			{
				const SpawnTypeComp* victimSpawnType =
					ctx.ecs.GetComponent<SpawnTypeComp>(entity);
				const CharacterDef* victimDef =
					victimSpawnType != nullptr
						? GameDataCatalog::Current().Characters().Find(
							victimSpawnType->characterId)
						: nullptr;

				if (victimDef != nullptr)
				{
					for (const KillBuffGrantDef& grant : victimDef->killBuffGrants)
					{
						if (!RollKillBuffGrant(grant.grantProbability))
						{
							continue;
						}

						const GameplayEffectDef* effectDef =
							GameplayContentCatalogSnapshot::Current()
								.FindEffectByKey(grant.buffEffectKey);
						if (effectDef != nullptr)
						{
							killerBuffComp->pendingBuffEffectIds.push_back(
								effectDef->id);
						}
					}
				}
			}
		}

		// 전투 통계 이벤트 예약: 플레이어↔몬스터 간 킬 블로우 발생 시 세팅.
		if (!killerEntity.IsNull() && stats.currentHp <= 0)
		{
			const bool victimIsPlayer =
				ctx.ecs.GetComponent<PlayerControlIdentityComp>(entity) != nullptr;
			const bool killerIsPlayer =
				ctx.ecs.GetComponent<PlayerControlIdentityComp>(killerEntity) != nullptr;

			const SpawnTypeComp* victimSpawnType =
				ctx.ecs.GetComponent<SpawnTypeComp>(entity);
			const SpawnTypeComp* killerSpawnType =
				ctx.ecs.GetComponent<SpawnTypeComp>(killerEntity);

			// 플레이어가 몬스터를 처치 → 처치 카운트 이벤트 예약.
			if (killerIsPlayer && !victimIsPlayer && victimSpawnType != nullptr)
			{
				if (PendingMonsterKillEventComp* killEvent =
					ctx.ecs.GetMutableComponent<PendingMonsterKillEventComp>(killerEntity))
				{
					killEvent->killedCharacterId = victimSpawnType->characterId;
					killEvent->pending           = true;
				}
			}

			// 몬스터가 플레이어를 처치 → 사망 카운트 이벤트 예약.
			if (victimIsPlayer && !killerIsPlayer && killerSpawnType != nullptr)
			{
				if (PendingPlayerDeathCountEventComp* deathEvent =
					ctx.ecs.GetMutableComponent<PendingPlayerDeathCountEventComp>(entity))
				{
					deathEvent->killerCharacterId = killerSpawnType->characterId;
					deathEvent->pending           = true;
				}
			}
		}

		if (previousStats.currentStamina > stats.currentStamina)
		{
			StaminaRecoveryStateComp* recovery =
				ctx.ecs.GetMutableComponent<StaminaRecoveryStateComp>(entity);
			ApplyStaminaRecoveryDelay(
				ctx,
				entity,
				stats,
				recovery != nullptr
					? recovery->tuning.damageRegenDelaySec
					: StaminaRecoveryTuning{}.damageRegenDelaySec);
		}

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
			AIPhaseRuntimeComp* bossPhase =
				ctx.ecs.GetMutableComponent<AIPhaseRuntimeComp>(entity);
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
					transitionIndex < profile->phaseTransitions.size() &&
					transitionIndex < 32u;
					++transitionIndex)
				{
					const AIPhaseTransitionDef& transition =
						profile->phaseTransitions[transitionIndex];
					const uint32_t thresholdMask =
						1u << static_cast<uint32_t>(transitionIndex);
					if ((bossPhase->crossedThresholdMask & thresholdMask) != 0 ||
						(transition.fromPhase.has_value() &&
							*transition.fromPhase != bossPhase->currentPhase) ||
						previousHpRatio <= transition.hpRatio ||
						currentHpRatio > transition.hpRatio)
					{
						continue;
					}

					bossPhase->crossedThresholdMask |= thresholdMask;
					bossPhase->currentPhase = transition.toPhase;
					bossPhase->transitionRequested = true;
					bossPhase->pendingTransitionIndex =
						static_cast<uint16_t>(transitionIndex);

					if (transition.toPhase == 2 &&
						aiType->aiType == AIArchetype::FinalBossMonster)
					{
						if (BossGimmickStateComp* gimmick =
							ctx.ecs.GetMutableComponent<BossGimmickStateComp>(entity))
						{
							gimmick->Request(BossGimmickType::PhaseTransitionObjects);
						}
					}

					if (AIActionRuntimeComp* actionRuntime =
						ctx.ecs.GetMutableComponent<AIActionRuntimeComp>(entity))
					{
						if (transition.clearActionCooldowns)
						{
							std::fill(
								actionRuntime->actionCooldownSec.begin(),
								actionRuntime->actionCooldownSec.end(),
								0.0f);
						}
						if (transition.clearGroupCooldowns)
						{
							std::fill(
								actionRuntime->groupCooldownSec.begin(),
								actionRuntime->groupCooldownSec.end(),
								0.0f);
						}
						actionRuntime->basicActionCountSinceEffect = 0;
						actionRuntime->globalActionCooldownSec =
							std::max(
								actionRuntime->globalActionCooldownSec,
								transition.transitionLockSec);
						actionRuntime->movementLockSec =
							std::max(
								actionRuntime->movementLockSec,
								transition.transitionLockSec);
					}

					if (AIMovementRuntimeComp* movementRuntime =
						ctx.ecs.GetMutableComponent<AIMovementRuntimeComp>(entity))
					{
						movementRuntime->strafeTimeLeftSec = 0.0f;
					}

					if (transition.forceRetarget)
					{
						if (AIBlackboardComp* blackboard =
							ctx.ecs.GetMutableComponent<AIBlackboardComp>(entity))
						{
							if (!result.reactionSource.IsNull())
							{
								blackboard->lastAttacker = result.reactionSource;
								blackboard->currentTarget = result.reactionSource;
							}
							blackboard->forceRetarget = true;
						}
					}

					if (auto* aiReaction =
						ctx.ecs.GetMutableComponent<AIReactionEventQueueComp>(entity))
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
			if (auto* aiReaction = ctx.ecs.GetMutableComponent<AIReactionEventQueueComp>(entity))
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

			if (auto* aiReaction = ctx.ecs.GetMutableComponent<AIReactionEventQueueComp>(interaction.sourceEntity))
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

		if (previousStats.currentHp > 0 && stats.currentHp <= 0)
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
			if (PendingGameplayEffectApplyComp* effectApplyComp =
				ctx.ecs.GetMutableComponent<PendingGameplayEffectApplyComp>(
					entity))
			{
				effectApplyComp->effectId = *result.pendingParryEffectId;
			}
		}
	}
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
