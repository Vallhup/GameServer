#include "pch.h"
#include "ResolveDeathAndDespawnSystem.h"

#include "../GameplaySystemUtil.h"
#include "WorldDef.h"

using namespace GameplaySystemUtil;

namespace
{
	bool TryResolveDefaultPlayerSpawnTransform(
		const WorldDef* worldDef,
		DirectX::XMFLOAT3& outPosition,
		DirectX::XMFLOAT4& outRotation) noexcept
	{
		if (worldDef == nullptr ||
			worldDef->map.defaultPlayerSpawnPointId == SpawnPointIds::None)
		{
			return false;
		}

		for (const SpawnPointDef& spawnPoint : worldDef->map.spawnPoints)
		{
			if (spawnPoint.id != worldDef->map.defaultPlayerSpawnPointId)
			{
				continue;
			}

			outPosition = DirectX::XMFLOAT3{
				spawnPoint.position.x,
				spawnPoint.position.y,
				spawnPoint.position.z
			};
			outRotation = DirectX::XMFLOAT4{
				spawnPoint.rotation.x,
				spawnPoint.rotation.y,
				spawnPoint.rotation.z,
				spawnPoint.rotation.w
			};
			return true;
		}

		return false;
	}

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

		DirectX::XMFLOAT3 respawnPosition{};
		DirectX::XMFLOAT4 respawnRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
		const bool hasRespawnTransform =
			TryResolveDefaultPlayerSpawnTransform(
				ctx.runtime.GetDef(),
				respawnPosition,
				respawnRotation);

		if (WorldTransformComp* const transform =
			ctx.ecs.GetMutableComponent<WorldTransformComp>(entity))
		{
			if (hasRespawnTransform)
			{
				transform->position = respawnPosition;
				transform->rotation = respawnRotation;
			}

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
		}

		if (PendingPlayerDeathCountEventComp* const deathEvent =
			ctx.ecs.GetMutableComponent<PendingPlayerDeathCountEventComp>(entity))
		{
			deathEvent->pending = false;
		}

		deathState = PlayerDeathStateComp{};

		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Stat);
		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Transform);
		MarkDirtyIfPresent(ctx, entity, WorldDirtyType::Animation);

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

const StaticSystemMetaStorage<16> ResolveDeathAndDespawnSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveDeathAndDespawnSystem>(),
		"ResolveDeathAndDespawnSystem",
		std::array<AccessSpec, 16>
	{
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<PlayerDeathCountConsumedTag>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferComp>()),
		WriteImmediate(ComponentRes<PendingPlayerDeathCountEventComp>()),
		WriteImmediate(ComponentRes<PlayerDeathStateComp>()),
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<AnimationPlaybackStateComp>()),
		WriteImmediate(ComponentRes<LocomotionMoveDeltaComp>()),
		WriteImmediate(ComponentRes<AbilityMoveDeltaComp>()),
		WriteImmediate(ComponentRes<PreCollisionTransformComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		WriteDeferred(CommandBufferRes()),
	});

void ResolveDeathAndDespawnSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, stats, abilityState] :
		ctx.ecs.MutableView<CombatStatStateComp, AbilityStateComp>())
	{
		if (stats.currentHp > 0)
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
				ctx.runtime.DeferredRemoveComponent<PlayerDeathCountConsumedTag>(
					entity);
			}
			continue;
		}

		const PlayerControlIdentityComp* const player =
			ctx.ecs.GetComponent<PlayerControlIdentityComp>(entity);
		PlayerDeathStateComp* const deathState =
			ctx.ecs.GetMutableComponent<PlayerDeathStateComp>(entity);
		if (player != nullptr &&
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
			}
			else
			{
				ctx.runtime.DeferredUpsertComponent<
					PendingPlayerDeathCountEventComp>(
					entity,
					PendingPlayerDeathCountEventComp{ .pending = true });
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

		if (deadAbilityFinished &&
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

		if (deadAbilityFinished &&
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
