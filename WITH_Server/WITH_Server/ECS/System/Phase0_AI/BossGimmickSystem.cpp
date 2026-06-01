#include "pch.h"
#include "BossGimmickSystem.h"

#include <random>

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	constexpr float kPhaseTransitionTelegraphSec = 1.5f;
	constexpr float kPhaseTransitionObjectWindowSec = 8.0f;
	constexpr float kPhaseTransitionResolveSec = 1.0f;
	constexpr float kPhaseTransitionObjectHp = 60.0f;
	constexpr float kPhaseTransitionObjectRadius = 5.5f;
	constexpr float kPhaseTransitionObjectRadiusJitter = 2.0f;
	constexpr float kPhaseTransitionObjectHeight = 2.0f;
	constexpr float kPhaseTransitionObjectHalfWidth = 0.75f;
	constexpr float kPhaseTransitionImmunitySec = 12.0f;

	constexpr float kFinalSafeZoneTelegraphSec = 5.0f;
	constexpr float kFinalSafeZoneActiveSec = 4.0f;
	constexpr float kFinalSafeZoneResolveSec = 0.5f;
	constexpr float kFinalSafeZoneRadius = 1.25f;
	constexpr float kFinalSafeZoneDistance = 5.5f;
	constexpr float kFinalSafeZoneDistanceJitter = 1.5f;
	constexpr float kBossLockRefreshSec = 0.25f;

	struct AlivePlayerEntry
	{
		Entity entity{ Entity::Null() };
		XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	};

	float RandomRange(float minValue, float maxValue)
	{
		thread_local std::mt19937 rng{ std::random_device{}() };
		std::uniform_real_distribution<float> dist(minValue, maxValue);
		return dist(rng);
	}

	std::vector<AlivePlayerEntry> CollectAlivePlayers(SystemContext& ctx)
	{
		std::vector<AlivePlayerEntry> players;
		for (auto [entity, player, transform, stats] :
			ctx.ecs.View<
				PlayerControlIdentityComp,
				WorldTransformComp,
				CombatStatStateComp>())
		{
			if (player.ownerSessionId != 0 &&
				stats.currentHp > 0 &&
				!HasBlockingPendingState(ctx.ecs, entity))
			{
				players.push_back(AlivePlayerEntry{ entity, transform.position });
			}
		}
		std::sort(players.begin(), players.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.entity.id < rhs.entity.id;
		});
		return players;
	}

	bool ContainsEntity(const std::vector<Entity>& entities, Entity entity)
	{
		return std::find(entities.begin(), entities.end(), entity) !=
			entities.end();
	}

	void MarkDirtyIfPresent(
		SystemContext& ctx,
		Entity entity,
		WorldDirtyType dirtyType)
	{
		if (DirtyFlagsComp* dirty =
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
		{
			dirty->MarkDirty(dirtyType);
		}
	}

	void SetStage(
		BossGimmickStateComp& gimmick,
		BossGimmickStage stage,
		float durationSec)
	{
		gimmick.stage = stage;
		gimmick.stageElapsedSec = 0.0f;
		gimmick.stageDurationSec = std::max(0.0f, durationSec);
	}

	uint32_t ToSyncValue(BossGimmickType type) noexcept
	{
		return static_cast<uint32_t>(type);
	}

	uint32_t ToSyncValue(BossGimmickStage stage) noexcept
	{
		return static_cast<uint32_t>(stage);
	}

	uint32_t ToSyncValue(BossGimmickObjectSyncState state) noexcept
	{
		return static_cast<uint32_t>(state);
	}

	uint32_t ClampHp(int32_t hp) noexcept
	{
		return static_cast<uint32_t>(std::max(0, hp));
	}

	uint64_t ToGimmickNetId(Entity entity) noexcept
	{
		return entity.IsNull() ? 0ull : entity.id;
	}

	float RemainingStageSec(const BossGimmickStateComp& gimmick) noexcept
	{
		return std::max(
			0.0f,
			gimmick.stageDurationSec - gimmick.stageElapsedSec);
	}

	void EmitStateSync(
		SystemContext& ctx,
		Entity boss,
		const BossGimmickStateComp& gimmick)
	{
		PendingBossGimmickReplicationComp* pending =
			ctx.ecs.GetMutableComponent<PendingBossGimmickReplicationComp>(
				boss);
		if (pending == nullptr)
			return;

		pending->stateEvents.push_back(
			PendingBossGimmickStateSyncEvent{
				.boss = boss,
				.gimmickSeq = gimmick.gimmickSeq,
				.gimmickType = ToSyncValue(gimmick.activeType),
				.stage = ToSyncValue(gimmick.stage),
				.durationSec = gimmick.stageDurationSec,
				.remainingSec = RemainingStageSec(gimmick)
			});
	}

	void EmitObjectSync(
		SystemContext& ctx,
		Entity boss,
		const BossGimmickStateComp& gimmick,
		Entity object,
		BossGimmickObjectSyncState state,
		const XMFLOAT3& position,
		float radius,
		int32_t curHp,
		int32_t maxHp)
	{
		PendingBossGimmickReplicationComp* pending =
			ctx.ecs.GetMutableComponent<PendingBossGimmickReplicationComp>(
				boss);
		if (pending == nullptr)
			return;

		pending->objectEvents.push_back(
			PendingBossGimmickObjectSyncEvent{
				.boss = boss,
				.gimmickSeq = gimmick.gimmickSeq,
				.objectNetId = ToGimmickNetId(object),
				.state = state,
				.position = position,
				.radius = radius,
				.curHp = ClampHp(curHp),
				.maxHp = ClampHp(maxHp)
			});
	}

	void EmitZoneSync(
		SystemContext& ctx,
		Entity boss,
		const BossGimmickStateComp& gimmick,
		Entity zone,
		BossGimmickObjectSyncState state,
		const XMFLOAT3& position,
		float radius)
	{
		PendingBossGimmickReplicationComp* pending =
			ctx.ecs.GetMutableComponent<PendingBossGimmickReplicationComp>(
				boss);
		if (pending == nullptr)
			return;

		pending->zoneEvents.push_back(
			PendingBossGimmickZoneSyncEvent{
				.boss = boss,
				.gimmickSeq = gimmick.gimmickSeq,
				.zoneNetId = ToGimmickNetId(zone),
				.state = state,
				.position = position,
				.radius = radius
			});
	}

	XMFLOAT3 BuildRingPosition(
		const XMFLOAT3& origin,
		size_t index,
		size_t count,
		float baseRadius,
		float radiusJitter)
	{
		const float step = count > 0
			? (2.0f * kPi) / static_cast<float>(count)
			: 0.0f;
		const float angle = step * static_cast<float>(index) +
			RandomRange(-0.35f, 0.35f);
		const float radius = baseRadius + RandomRange(0.0f, radiusJitter);
		return XMFLOAT3{
			origin.x + std::cos(angle) * radius,
			origin.y,
			origin.z + std::sin(angle) * radius
		};
	}

	Entity SpawnGimmickObject(
		SystemContext& ctx,
		Entity boss,
		Entity assignedPlayer,
		const XMFLOAT3& position)
	{
		const Entity object = ctx.runtime.ReserveEntity();
		if (object.IsNull())
			return Entity::Null();

		ctx.runtime.DeferredAddComponent<WorldTransformComp>(
			object,
			WorldTransformComp{
				.position = position,
				.rotation = XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f },
				.scale = XMFLOAT3{ 1.0f, 1.0f, 1.0f }
			});
		ctx.runtime.DeferredAddComponent<CombatStatStateComp>(
			object,
			CombatStatStateComp{
				.currentHp = static_cast<int32_t>(kPhaseTransitionObjectHp),
				.maxHp = static_cast<int32_t>(kPhaseTransitionObjectHp),
				.currentStamina = 0,
				.maxStamina = 0,
				.currentPoise = 0,
				.maxPoise = 0,
				.attackPower = 0,
				.defense = 0,
				.attackSpeed = 1.0f,
				.moveSpeed = 0.0f
			});
		ctx.runtime.DeferredAddComponent<PendingCombatResultComp>(
			object,
			PendingCombatResultComp{});
		ctx.runtime.DeferredAddComponent<GimmickObjectComp>(
			object,
			GimmickObjectComp{
				.ownerBoss = boss,
				.assignedPlayer = assignedPlayer,
				.lastSyncedHp = static_cast<int32_t>(
					kPhaseTransitionObjectHp)
			});
		ctx.runtime.DeferredAddComponent<StaticBoxHurtColliderComp>(
			object,
			StaticBoxHurtColliderComp{
				.halfExtents = XMFLOAT3{
					kPhaseTransitionObjectHalfWidth,
					kPhaseTransitionObjectHeight * 0.5f,
					kPhaseTransitionObjectHalfWidth
				}
			});
		return object;
	}

	Entity SpawnSafeZone(
		SystemContext& ctx,
		Entity boss,
		const XMFLOAT3& position)
	{
		const Entity safeZone = ctx.runtime.ReserveEntity();
		if (safeZone.IsNull())
			return Entity::Null();

		ctx.runtime.DeferredAddComponent<WorldTransformComp>(
			safeZone,
			WorldTransformComp{
				.position = position,
				.rotation = XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f },
				.scale = XMFLOAT3{ 1.0f, 1.0f, 1.0f }
			});
		ctx.runtime.DeferredAddComponent<SafeZoneComp>(
			safeZone,
			SafeZoneComp{
				.ownerBoss = boss,
				.radius = kFinalSafeZoneRadius,
				.remainingSec = kFinalSafeZoneActiveSec
			});
		return safeZone;
	}

	void CleanupGimmickObjects(
		SystemContext& ctx,
		Entity boss,
		const BossGimmickStateComp& gimmick)
	{
		for (Entity object : gimmick.phaseTransitionObjectEntities)
		{
			if (object.IsNull() || HasBlockingPendingState(ctx.ecs, object))
				continue;

			const GimmickObjectComp* objectGimmick =
				ctx.ecs.GetComponent<GimmickObjectComp>(object);
			if (objectGimmick != nullptr && objectGimmick->broken)
				continue;

			const WorldTransformComp* transform =
				ctx.ecs.GetComponent<WorldTransformComp>(object);
			const CombatStatStateComp* stats =
				ctx.ecs.GetComponent<CombatStatStateComp>(object);
			if (transform != nullptr && stats != nullptr)
			{
				EmitObjectSync(
					ctx,
					boss,
					gimmick,
					object,
					BossGimmickObjectSyncState::Despawned,
					transform->position,
					kPhaseTransitionObjectHalfWidth,
					stats->currentHp,
					stats->maxHp);
			}

			ctx.runtime.DeferredDestroyEntityIfAlive(object);
		}
	}

	void CleanupSafeZones(
		SystemContext& ctx,
		Entity boss,
		const BossGimmickStateComp& gimmick)
	{
		for (Entity zone : gimmick.finalSafeZoneEntities)
		{
			if (zone.IsNull() || HasBlockingPendingState(ctx.ecs, zone))
				continue;

			const WorldTransformComp* transform =
				ctx.ecs.GetComponent<WorldTransformComp>(zone);
			const SafeZoneComp* safeZone =
				ctx.ecs.GetComponent<SafeZoneComp>(zone);
			if (transform != nullptr && safeZone != nullptr)
			{
				EmitZoneSync(
					ctx,
					boss,
					gimmick,
					zone,
					BossGimmickObjectSyncState::Despawned,
					transform->position,
					safeZone->radius);
			}

			ctx.runtime.DeferredDestroyEntityIfAlive(zone);
		}
	}

	void RefreshBossLock(SystemContext& ctx, Entity boss)
	{
		if (AIIntentFrameComp* intent =
			ctx.ecs.GetMutableComponent<AIIntentFrameComp>(boss))
		{
			intent->ClearAll();
		}

		if (AIActionRuntimeComp* actionRuntime =
			ctx.ecs.GetMutableComponent<AIActionRuntimeComp>(boss))
		{
			actionRuntime->globalActionCooldownSec = std::max(
			actionRuntime->globalActionCooldownSec,
				kBossLockRefreshSec);
			actionRuntime->movementLockSec = std::max(
				actionRuntime->movementLockSec,
				kBossLockRefreshSec);
		}

		if (AIMovementRuntimeComp* movementRuntime =
			ctx.ecs.GetMutableComponent<AIMovementRuntimeComp>(boss))
		{
			movementRuntime->strafeTimeLeftSec = 0.0f;
			movementRuntime->hasPathCorner = false;
		}
	}
}

const StaticSystemMetaStorage<17> BossGimmickSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<BossGimmickSystem>(),
		"BossGimmickSystem",
		std::array<AccessSpec, 17>
	{
		WriteImmediate(ComponentRes<BossGimmickStateComp>()),
		WriteImmediate(ComponentRes<AIActionRuntimeComp>()),
		WriteImmediate(ComponentRes<AIMovementRuntimeComp>()),
		WriteImmediate(ComponentRes<AIIntentFrameComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<GimmickObjectComp>()),
		WriteImmediate(ComponentRes<SafeZoneComp>()),
		WriteImmediate(ComponentRes<BossGimmickImmunityComp>()),
		WriteImmediate(ComponentRes<StaticBoxHurtColliderComp>()),
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		WriteImmediate(ComponentRes<PendingBossGimmickReplicationComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteDeferred(CommandBufferRes()),
	});

void BossGimmickSystem::Execute(SystemContext& ctx)
{
	const float dtSec = static_cast<float>(std::max(0.0, ctx.dtSec));

	for (auto [entity, immunity] : ctx.ecs.View<BossGimmickImmunityComp>())
	{
		immunity.remainingSec -= dtSec;
		if (immunity.remainingSec <= 0.0f)
		{
			ctx.runtime.DeferredRemoveComponent<BossGimmickImmunityComp>(entity);
		}
	}

	for (auto [entity, safeZone] : ctx.ecs.View<SafeZoneComp>())
	{
		safeZone.remainingSec -= dtSec;
		if (safeZone.remainingSec <= -kFinalSafeZoneResolveSec)
		{
			ctx.runtime.DeferredDestroyEntityIfAlive(entity);
		}
	}

	for (auto [boss, gimmick] : ctx.ecs.View<BossGimmickStateComp>())
	{
		if (gimmick.stage == BossGimmickStage::Completed)
		{
			gimmick.Complete();
			continue;
		}

		if (!gimmick.IsActive())
		{
			if (gimmick.finalGimmickRequested &&
				!gimmick.finalGimmickCompleted)
			{
				gimmick.Begin(
					BossGimmickType::FinalSafeZone,
					BossGimmickStage::Telegraph,
					kFinalSafeZoneTelegraphSec,
					true);
				EmitStateSync(ctx, boss, gimmick);
			}
			else if (gimmick.phaseTransitionGimmickRequested &&
				!gimmick.phaseTransitionGimmickCompleted)
			{
				gimmick.Begin(
					BossGimmickType::PhaseTransitionObjects,
					BossGimmickStage::Telegraph,
					kPhaseTransitionTelegraphSec,
					true);
				EmitStateSync(ctx, boss, gimmick);
			}
			else
			{
				continue;
			}
		}

		gimmick.elapsedSec += dtSec;
		gimmick.stageElapsedSec += dtSec;

		switch (gimmick.activeType)
		{
		case BossGimmickType::PhaseTransitionObjects:
			TickPhaseTransitionObjects(ctx, boss, gimmick, dtSec);
			break;
		case BossGimmickType::FinalSafeZone:
			TickFinalSafeZone(ctx, boss, gimmick, dtSec);
			break;
		default:
			break;
		}

		if (gimmick.BlocksAI())
		{
			RefreshBossLock(ctx, boss);
		}
	}
}

void BossGimmickSystem::TickPhaseTransitionObjects(
	SystemContext& ctx,
	Entity boss,
	BossGimmickStateComp& gimmick,
	float dtSec)
{
	(void)dtSec;

	if (gimmick.stage == BossGimmickStage::Telegraph)
	{
		if (gimmick.stageElapsedSec >= gimmick.stageDurationSec)
		{
			SetStage(
				gimmick,
				BossGimmickStage::Active,
				kPhaseTransitionObjectWindowSec);
			EmitStateSync(ctx, boss, gimmick);
		}
		return;
	}

	const WorldTransformComp* bossTransform =
		ctx.ecs.GetComponent<WorldTransformComp>(boss);
	if (bossTransform == nullptr)
		return;

	if (gimmick.stage == BossGimmickStage::Active)
	{
		if (!gimmick.phaseTransitionObjectsSpawned)
		{
			const std::vector<AlivePlayerEntry> players =
				CollectAlivePlayers(ctx);
			for (size_t i = 0; i < players.size(); ++i)
			{
				const XMFLOAT3 position = 
					BuildRingPosition(bossTransform->position, i, players.size(),
						kPhaseTransitionObjectRadius, kPhaseTransitionObjectRadiusJitter);

				const Entity object = SpawnGimmickObject(
					ctx,
					boss,
					players[i].entity,
					position);
				if (!object.IsNull())
				{
					gimmick.phaseTransitionObjectEntities.push_back(object);
					EmitObjectSync(
						ctx,
						boss,
						gimmick,
						object,
						BossGimmickObjectSyncState::Spawned,
						position,
						kPhaseTransitionObjectHalfWidth,
						static_cast<int32_t>(kPhaseTransitionObjectHp),
						static_cast<int32_t>(kPhaseTransitionObjectHp));
				}
			}
			gimmick.phaseTransitionObjectsSpawned = true;

			if (players.empty())
			{
				SetStage(
					gimmick,
					BossGimmickStage::Resolve,
					kPhaseTransitionResolveSec);
				EmitStateSync(ctx, boss, gimmick);
			}
			return;
		}

		size_t brokenCount = 0;
		for (Entity object : gimmick.phaseTransitionObjectEntities)
		{
			GimmickObjectComp* objectGimmick =
				ctx.ecs.GetMutableComponent<GimmickObjectComp>(object);
			CombatStatStateComp* objectStats =
				ctx.ecs.GetMutableComponent<CombatStatStateComp>(object);
			if (objectGimmick == nullptr || objectStats == nullptr)
			{
				++brokenCount;
				continue;
			}

			if (!objectGimmick->broken && objectStats->currentHp <= 0)
			{
				objectGimmick->broken = true;
				const WorldTransformComp* transform =
					ctx.ecs.GetComponent<WorldTransformComp>(object);
				EmitObjectSync(
					ctx,
					boss,
					gimmick,
					object,
					BossGimmickObjectSyncState::Broken,
					transform != nullptr
						? transform->position
						: XMFLOAT3{ 0.0f, 0.0f, 0.0f },
					kPhaseTransitionObjectHalfWidth,
					0,
					objectStats->maxHp);
				const Entity immuneTarget =
					!objectGimmick->lastHitBy.IsNull()
						? objectGimmick->lastHitBy
						: objectGimmick->assignedPlayer;
				if (!immuneTarget.IsNull())
				{
					ctx.runtime.DeferredUpsertComponent<BossGimmickImmunityComp>(
						immuneTarget,
						BossGimmickImmunityComp{
							.ownerBoss = boss,
							.remainingSec = kPhaseTransitionImmunitySec
						});
					if (!ContainsEntity(
							gimmick.phaseTransitionImmunePlayers,
							immuneTarget))
					{
						gimmick.phaseTransitionImmunePlayers.push_back(immuneTarget);
					}
				}
				ctx.runtime.DeferredDestroyEntityIfAlive(object);
			}
			else if (!objectGimmick->broken &&
				objectStats->currentHp != objectGimmick->lastSyncedHp)
			{
				if (const WorldTransformComp* transform =
					ctx.ecs.GetComponent<WorldTransformComp>(object))
				{
					EmitObjectSync(
						ctx,
						boss,
						gimmick,
						object,
						BossGimmickObjectSyncState::Updated,
						transform->position,
						kPhaseTransitionObjectHalfWidth,
						objectStats->currentHp,
						objectStats->maxHp);
					objectGimmick->lastSyncedHp = objectStats->currentHp;
				}
			}

			if (objectGimmick->broken || objectStats->currentHp <= 0)
				++brokenCount;
		}

		if (brokenCount >= gimmick.phaseTransitionObjectEntities.size() ||
			gimmick.stageElapsedSec >= gimmick.stageDurationSec)
		{
			SetStage(
				gimmick,
				BossGimmickStage::Resolve,
				kPhaseTransitionResolveSec);
			EmitStateSync(ctx, boss, gimmick);
		}
		return;
	}

	if (gimmick.stage == BossGimmickStage::Resolve &&
		!gimmick.phaseTransitionInstantKillResolved)
	{
		for (const AlivePlayerEntry& player : CollectAlivePlayers(ctx))
		{
			const BossGimmickImmunityComp* immunity =
				ctx.ecs.GetComponent<BossGimmickImmunityComp>(player.entity);
			if (immunity != nullptr &&
				immunity->ownerBoss == boss &&
				immunity->remainingSec > 0.0f)
			{
				continue;
			}

			if (CombatStatStateComp* stats =
				ctx.ecs.GetMutableComponent<CombatStatStateComp>(player.entity))
			{
				stats->currentHp = 0;
				MarkDirtyIfPresent(ctx, player.entity, WorldDirtyType::Stat);
			}
		}

		CleanupGimmickObjects(ctx, boss, gimmick);
		gimmick.phaseTransitionInstantKillResolved = true;
	}

	if (gimmick.stage == BossGimmickStage::Resolve &&
		gimmick.stageElapsedSec >= gimmick.stageDurationSec)
	{
		SetStage(gimmick, BossGimmickStage::Completed, 0.0f);
		EmitStateSync(ctx, boss, gimmick);
	}
}

void BossGimmickSystem::TickFinalSafeZone(
	SystemContext& ctx,
	Entity boss,
	BossGimmickStateComp& gimmick,
	float dtSec)
{
	(void)dtSec;

	if (gimmick.stage == BossGimmickStage::Telegraph)
	{
		if (gimmick.stageElapsedSec >= gimmick.stageDurationSec)
		{
			SetStage(
				gimmick,
				BossGimmickStage::Active,
				kFinalSafeZoneActiveSec);
			EmitStateSync(ctx, boss, gimmick);
		}
		return;
	}

	const WorldTransformComp* bossTransform =
		ctx.ecs.GetComponent<WorldTransformComp>(boss);
	if (bossTransform == nullptr)
		return;

	if (gimmick.stage == BossGimmickStage::Active)
	{
		if (!gimmick.finalSafeZoneSpawned)
		{
			const XMFLOAT3 position = BuildRingPosition(
				bossTransform->position,
				0,
				1,
				kFinalSafeZoneDistance,
				kFinalSafeZoneDistanceJitter);
			const Entity safeZone = SpawnSafeZone(ctx, boss, position);
			if (!safeZone.IsNull())
			{
				gimmick.finalSafeZoneEntities.push_back(safeZone);
				EmitZoneSync(
					ctx,
					boss,
					gimmick,
					safeZone,
					BossGimmickObjectSyncState::Spawned,
					position,
					kFinalSafeZoneRadius);
			}
			gimmick.finalSafeZoneSpawned = true;
		}

		if (gimmick.stageElapsedSec >= gimmick.stageDurationSec)
		{
			SetStage(
				gimmick,
				BossGimmickStage::Resolve,
				kFinalSafeZoneResolveSec);
			EmitStateSync(ctx, boss, gimmick);
		}
		return;
	}

	if (gimmick.stage == BossGimmickStage::Resolve &&
		!gimmick.finalSafeZoneResolved)
	{
		Entity safeZoneEntity = Entity::Null();
		if (!gimmick.finalSafeZoneEntities.empty())
			safeZoneEntity = gimmick.finalSafeZoneEntities.front();

		const WorldTransformComp* safeZoneTransform =
			ctx.ecs.GetComponent<WorldTransformComp>(safeZoneEntity);
		const SafeZoneComp* safeZone =
			ctx.ecs.GetComponent<SafeZoneComp>(safeZoneEntity);

		for (const AlivePlayerEntry& player : CollectAlivePlayers(ctx))
		{
			bool insideSafeZone = false;
			if (safeZoneTransform != nullptr && safeZone != nullptr)
			{
				const float dx =
					player.position.x - safeZoneTransform->position.x;
				const float dz =
					player.position.z - safeZoneTransform->position.z;
				insideSafeZone = dx * dx + dz * dz <=
					safeZone->radius * safeZone->radius;
			}

			if (!insideSafeZone)
			{
				if (CombatStatStateComp* stats =
					ctx.ecs.GetMutableComponent<CombatStatStateComp>(
						player.entity))
				{
					stats->currentHp = 0;
					MarkDirtyIfPresent(
						ctx,
						player.entity,
						WorldDirtyType::Stat);
				}
			}
		}

		if (CombatStatStateComp* bossStats =
			ctx.ecs.GetMutableComponent<CombatStatStateComp>(boss))
		{
			bossStats->currentHp = 0;
			MarkDirtyIfPresent(ctx, boss, WorldDirtyType::Stat);
		}

		CleanupSafeZones(ctx, boss, gimmick);
		gimmick.finalSafeZoneResolved = true;
	}

	if (gimmick.stage == BossGimmickStage::Resolve &&
		gimmick.stageElapsedSec >= gimmick.stageDurationSec)
	{
		SetStage(gimmick, BossGimmickStage::Completed, 0.0f);
		EmitStateSync(ctx, boss, gimmick);
	}
}
