#include "pch.h"
#include "ResolveDeathAndDespawnSystem.h"

#include "../GameplaySystemUtil.h"
#include "PlayerDeathStatePolicy.h"
#include "WorldDef.h"

using namespace GameplaySystemUtil;

namespace
{
	void MarkDirtyIfPresent(
		SystemContext& ctx,
		Entity entity,
		WorldDirtyType dirtyType)
	{
		if (DirtyFlagsComp* const dirty =
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
		{
			dirty->MarkDirty(dirtyType);
		}
	}

	void ResetPlayerForRespawn(
		SystemContext& ctx,
		Entity entity,
		CombatStatStateComp& stats,
		AbilityStateComp& abilityState,
		PlayerDeathStateComp& deathState)
	{
		stats.currentHp = stats.maxHp > 0 ? stats.maxHp : 1;
		stats.currentStamina =
			stats.maxStamina > 0 ? stats.maxStamina : stats.currentStamina;
		stats.currentPoise =
			stats.maxPoise > 0 ? stats.maxPoise : stats.currentPoise;

		const uint32_t nextAbilityInstanceId =
			abilityState.abilityInstanceId + 1;
		abilityState = AbilityStateComp{};
		abilityState.abilityInstanceId = nextAbilityInstanceId;

		if (AbilityInterruptQueueComp* const interruptQueue =
			ctx.ecs.GetMutableComponent<AbilityInterruptQueueComp>(entity))
		{
			interruptQueue->events.clear();
		}

		if (AbilityTimelineAdvanceComp* const advance =
			ctx.ecs.GetMutableComponent<AbilityTimelineAdvanceComp>(entity))
		{
			*advance = AbilityTimelineAdvanceComp{};
		}

		if (ActorInputComp* const input =
			ctx.ecs.GetMutableComponent<ActorInputComp>(entity))
		{
			*input = ActorInputComp{};
		}

		if (LocomotionStateComp* const locomotion =
			ctx.ecs.GetMutableComponent<LocomotionStateComp>(entity))
		{
			const float facingYawRad = locomotion->facingYawRad;
			*locomotion = LocomotionStateComp{};
			locomotion->facingYawRad = facingYawRad;
			locomotion->desiredFacingYawRad = facingYawRad;
		}

		if (StaminaRecoveryStateComp* const recovery =
			ctx.ecs.GetMutableComponent<StaminaRecoveryStateComp>(entity))
		{
			recovery->regenLockRemainingSec = 0.0f;
			recovery->regenRemainder = 0.0f;
		}

		if (ConsumableInventoryComp* const inventory =
			ctx.ecs.GetMutableComponent<ConsumableInventoryComp>(entity))
		{
			inventory->hpPotionCount = HpPotionTuning{}.defaultGrantCount;
		}

		if (const WorldTransformComp* const transform =
			ctx.ecs.GetComponent<WorldTransformComp>(entity))
		{
			if (PreCollisionTransformComp* const preCollision =
				ctx.ecs.GetMutableComponent<PreCollisionTransformComp>(entity))
			{
				preCollision->prevPosition = transform->position;
				preCollision->prevRotation = transform->rotation;
				preCollision->candidatePosition = transform->position;
				preCollision->candidateRotation = transform->rotation;
				preCollision->movedThisFrame = false;
				preCollision->rotatedThisFrame = false;
				preCollision->preserveAbilityVerticalAboveNavMesh = false;
			}
		}

		if (LocomotionMoveDeltaComp* const locomotionDelta =
			ctx.ecs.GetMutableComponent<LocomotionMoveDeltaComp>(entity))
		{
			*locomotionDelta = LocomotionMoveDeltaComp{};
		}

		if (AbilityMoveDeltaComp* const abilityDelta =
			ctx.ecs.GetMutableComponent<AbilityMoveDeltaComp>(entity))
		{
			*abilityDelta = AbilityMoveDeltaComp{};
		}

		if (AnimationPlaybackStateComp* const playback =
			ctx.ecs.GetMutableComponent<AnimationPlaybackStateComp>(entity))
		{
			*playback = AnimationPlaybackStateComp{};
			if (const SpawnTypeComp* const spawnType =
				ctx.ecs.GetComponent<SpawnTypeComp>(entity))
			{
				playback->source = AnimationPlaybackSource::Locomotion;
				playback->animationId = ResolveLocomotionAnimationId(
					spawnType->characterId,
					LocomotionMode::Idle);
				playback->boundLocomotionMode = LocomotionMode::Idle;
				playback->playRate = 1.0f;
				playback->loop = false;
				playback->holdLastFrame = true;
			}
		}

		if (PendingPlayerDeathCountEventComp* const deathEvent =
			ctx.ecs.GetMutableComponent<PendingPlayerDeathCountEventComp>(entity))
		{
			// killerCharacterId 포함 전체 초기화: 리스폰 후 이전 사망 정보가 잔류하지 않도록 한다.
			*deathEvent = PendingPlayerDeathCountEventComp{};
		}

		deathState = PlayerDeathStateComp{};

		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Stat);
		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Transform);
		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Animation);
		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Inventory);

		if (ctx.ecs.HasComponent<PlayerDeathCountConsumedTag>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PlayerDeathCountConsumedTag>(
				entity);
		}
		if (ctx.ecs.HasComponent<PendingWorldTransferTag>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PendingWorldTransferTag>(entity);
		}
		if (ctx.ecs.HasComponent<PendingWorldTransferComp>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PendingWorldTransferComp>(entity);
		}
	}
}

const StaticSystemMetaStorage<24> ResolveDeathAndDespawnSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveDeathAndDespawnSystem>(),
		"ResolveDeathAndDespawnSystem",
		std::array<AccessSpec, 24>
	{
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<AbilityStateComp>()),
		WriteImmediate(ComponentRes<AbilityInterruptQueueComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<PlayerDeathCountConsumedTag>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferComp>()),
		WriteImmediate(ComponentRes<PendingPlayerDeathCountEventComp>()),
		WriteImmediate(ComponentRes<PlayerDeathStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<AnimationPlaybackStateComp>()),
		WriteImmediate(ComponentRes<LocomotionMoveDeltaComp>()),
		WriteImmediate(ComponentRes<AbilityMoveDeltaComp>()),
		WriteImmediate(ComponentRes<PreCollisionTransformComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		WriteImmediate(ComponentRes<ConsumableInventoryComp>()),
		WriteImmediate(ComponentRes<ActorInputComp>()),
		WriteImmediate(ComponentRes<LocomotionStateComp>()),
		WriteImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		WriteImmediate(ComponentRes<StaminaRecoveryStateComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		WriteImmediate(ComponentRes<PendingFinalBossDefeatedEventComp>()),
		WriteDeferred(CommandBufferRes()),
	});

void ResolveDeathAndDespawnSystem::Execute(SystemContext& ctx)
{
	const WorldDef* const worldDef = ctx.runtime.GetDef();
	const bool supportsPlayerRespawn =
		worldDef != nullptr &&
		PlayerDeathStatePolicy::IsRespawnWorld(worldDef->id);
	// PvP에서는 패배한 플레이어를 despawn 하지 않고 사망 자세 그대로 둔다.
	// 라운드 종료(라스트맨) 시 생존자/사망자 전원을 함께 Plaza로 전이시킨다.
	const bool isPvpWorld =
		worldDef != nullptr && worldDef->id == WorldDefId::Pvp;

	for (auto [entity, stats, abilityState] :
		ctx.ecs.MutableView<CombatStatStateComp, AbilityStateComp>())
	{
		if (stats.currentHp > 0)
		{
			if (supportsPlayerRespawn)
			{
				if (PlayerDeathStateComp* const deathState =
					ctx.ecs.GetMutableComponent<PlayerDeathStateComp>(entity);
					deathState != nullptr &&
					deathState->state != PlayerDeathState::Alive)
				{
					*deathState = PlayerDeathStateComp{};
				}
				if (ctx.ecs.HasComponent<PlayerDeathCountConsumedTag>(entity))
				{
					ctx.runtime.DeferredRemoveComponent<
						PlayerDeathCountConsumedTag>(entity);
				}
			}
			continue;
		}

		const PlayerControlIdentityComp* const player =
			ctx.ecs.GetComponent<PlayerControlIdentityComp>(entity);
		PlayerDeathStateComp* const deathState =
			ctx.ecs.GetMutableComponent<PlayerDeathStateComp>(entity);
		if (supportsPlayerRespawn &&
			player != nullptr &&
			player->ownerSessionId != 0 &&
			deathState != nullptr &&
			deathState->state == PlayerDeathState::Alive &&
			!ctx.ecs.HasComponent<PlayerDeathCountConsumedTag>(entity))
		{
			deathState->state = PlayerDeathState::WaitingForDeathCount;
			deathState->respawnRequested = false;
			if (PendingPlayerDeathCountEventComp* const deathEvent =
				ctx.ecs.GetMutableComponent<PendingPlayerDeathCountEventComp>(
					entity))
			{
				deathEvent->pending = true;
				deathEvent->decisionPending = true;
			}
			else
			{
				ctx.runtime.DeferredUpsertComponent<
					PendingPlayerDeathCountEventComp>(
					entity,
					PendingPlayerDeathCountEventComp{
						.pending = true,
						.decisionPending = true });
			}

			ctx.runtime.DeferredAddComponent<PlayerDeathCountConsumedTag>(
				entity,
				PlayerDeathCountConsumedTag{});
		}

		const AbilityDef* abilityDef =
			IsAbilityActive(abilityState)
			? GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId)
			: nullptr;
		const bool deadAbilityFinished =
			abilityDef != nullptr &&
			abilityDef->kind == AbilityKind::Dead &&
			abilityState.elapsedSec >= abilityDef->timeline.durationSec;
		PendingFinalBossDefeatedEventComp* const finalBossDefeated =
			ctx.ecs.GetMutableComponent<PendingFinalBossDefeatedEventComp>(
				entity);
		if (deadAbilityFinished &&
			finalBossDefeated != nullptr &&
			finalBossDefeated->awaitingDeathAnimation)
		{
			finalBossDefeated->awaitingDeathAnimation = false;
			finalBossDefeated->pending = true;
		}

		if (supportsPlayerRespawn &&
			deadAbilityFinished &&
			player != nullptr &&
			player->ownerSessionId != 0)
		{
			if (deathState != nullptr &&
				deathState->state == PlayerDeathState::AwaitingRespawnInput &&
				deathState->respawnRequested)
			{
				ResetPlayerForRespawn(
					ctx,
					entity,
					stats,
					abilityState,
					*deathState);
			}
			continue;
		}

		// PvP 월드의 플레이어 시신은 despawn하지 않고 그대로 유지한다(라운드 종료
		// 시 전원 전송). 몬스터 등 비플레이어나 다른 월드는 기존대로 despawn.
		const bool keepPvpCorpse =
			isPvpWorld &&
			player != nullptr &&
			player->ownerSessionId != 0;

		if (deadAbilityFinished &&
			(finalBossDefeated == nullptr || !finalBossDefeated->pending) &&
			!keepPvpCorpse &&
			!ctx.ecs.HasComponent<PendingDespawnTag>(entity))
		{
			ctx.runtime.DeferredAddComponent<PendingDespawnTag>(
				entity,
				PendingDespawnTag{});
			ctx.runtime.DeferredDestroyEntityIfAlive(entity);
		}
		if (ctx.ecs.HasComponent<PendingWorldTransferTag>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PendingWorldTransferTag>(entity);
		}
		if (ctx.ecs.HasComponent<PendingWorldTransferComp>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PendingWorldTransferComp>(entity);
		}
	}
}
