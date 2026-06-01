#include "pch.h"
#include "FrameworkFrameEventHarvester.h"

#include <cmath>
#include <unordered_set>

#include "ECS/GameplayRuntimeComponents.h"
#include "NetIdRegistry.h"
#include "RepComponent.h"
#include "WorldInstance.h"
#include "WorldLifecycleCommand.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldRuntime.h"

namespace
{
	float SafeFinite(float value) noexcept
	{
		return std::isfinite(value) ? value : 0.0f;
	}

	void SanitizeDirection(float& x, float& y, float& z) noexcept
	{
		x = SafeFinite(x);
		y = SafeFinite(y);
		z = SafeFinite(z);

		const float lengthSq = x * x + y * y + z * z;
		if (lengthSq <= 1.0e-6f || !std::isfinite(lengthSq))
		{
			x = 0.0f;
			y = 0.0f;
			z = -1.0f;
			return;
		}

		const float invLength = 1.0f / std::sqrt(lengthSq);
		x *= invLength;
		y *= invLength;
		z *= invLength;
	}
}

void FrameworkFrameEventHarvester::Harvest(
	WorldManager& worldManager,
	WorldRegistry& worldRegistry,
	NetIdRegistry& netIdRegistry,
	FrameworkRuntime::FrameResult::FrameEvents& outEvents)
{
	outEvents.Clear();

	for (WorldId worldId : worldManager.GetRunnableWorldIds())
	{
		WorldInstance* world = worldRegistry.FindWorld(worldId);
		if (world == nullptr)
		{
			continue;
		}

		WorldRuntime& runtime = world->GetRuntime();
		ECSView view = runtime.MakeView();

		for (auto [entity, impactEvents] :
			view.MutableView<PendingCombatImpactEventComp>())
		{
			(void)entity;

			for (const PendingCombatImpactEvent& impactEvent :
				impactEvents.events)
			{
				const NetId attackerNetId =
					netIdRegistry.FindNetId(
						worldId,
						impactEvent.sourceEntity);
				const NetId victimNetId =
					netIdRegistry.FindNetId(
						worldId,
						impactEvent.targetEntity);
				if (!attackerNetId.IsValid() || !victimNetId.IsValid())
				{
					continue;
				}

				FrameworkRuntime::FrameResult::CombatImpactEvent frameEvent{};
				frameEvent.worldId = worldId;
				frameEvent.attackerNetId = attackerNetId;
				frameEvent.victimNetId = victimNetId;
				frameEvent.resultType =
					static_cast<uint32_t>(impactEvent.resultType);
				frameEvent.impactX = SafeFinite(impactEvent.impactPoint.x);
				frameEvent.impactY = SafeFinite(impactEvent.impactPoint.y);
				frameEvent.impactZ = SafeFinite(impactEvent.impactPoint.z);
				frameEvent.dirX = impactEvent.swingDirection.x;
				frameEvent.dirY = impactEvent.swingDirection.y;
				frameEvent.dirZ = impactEvent.swingDirection.z;
				SanitizeDirection(
					frameEvent.dirX,
					frameEvent.dirY,
					frameEvent.dirZ);
				outEvents.combatImpacts.push_back(frameEvent);
			}

			impactEvents.events.clear();
		}

		for (auto [entity, deathEvent, player] :
			view.MutableView<
				PendingPlayerDeathCountEventComp,
				PlayerControlIdentityComp>())
		{
			if (!deathEvent.pending || player.ownerSessionId == 0)
			{
				continue;
			}

			const NetId netId = netIdRegistry.FindNetId(worldId, entity);
			outEvents.playerDeathCounts.push_back(
				FrameworkRuntime::FrameResult::PlayerDeathCountEvent{
					.worldId = worldId,
					.entity = entity,
					.sessionId = player.ownerSessionId,
					.netId = netId
				});

			deathEvent.pending = false;
		}

		for (auto [owner, bossGimmickEvents] :
			view.MutableView<PendingBossGimmickReplicationComp>())
		{
			for (const PendingBossGimmickObjectSyncEvent& objectEvent :
				bossGimmickEvents.objectEvents)
			{
				const Entity boss = !objectEvent.boss.IsNull()
					? objectEvent.boss
					: owner;
				const NetId bossNetId =
					netIdRegistry.FindNetId(worldId, boss);
				if (!bossNetId.IsValid() || objectEvent.objectNetId == 0)
				{
					continue;
				}

				FrameworkRuntime::FrameResult::BossGimmickObjectSyncEvent frameEvent{};
				frameEvent.worldId = worldId;
				frameEvent.bossNetId = bossNetId;
				frameEvent.gimmickSeq = objectEvent.gimmickSeq;
				frameEvent.objectNetId = objectEvent.objectNetId;
				frameEvent.state = static_cast<uint32_t>(objectEvent.state);
				frameEvent.x = SafeFinite(objectEvent.position.x);
				frameEvent.y = SafeFinite(objectEvent.position.y);
				frameEvent.z = SafeFinite(objectEvent.position.z);
				frameEvent.radius = SafeFinite(objectEvent.radius);
				frameEvent.curHp = objectEvent.curHp;
				frameEvent.maxHp = objectEvent.maxHp;
				outEvents.bossGimmickObjects.push_back(frameEvent);
			}

			for (const PendingBossGimmickZoneSyncEvent& zoneEvent :
				bossGimmickEvents.zoneEvents)
			{
				const Entity boss = !zoneEvent.boss.IsNull()
					? zoneEvent.boss
					: owner;
				const NetId bossNetId =
					netIdRegistry.FindNetId(worldId, boss);
				if (!bossNetId.IsValid() || zoneEvent.zoneNetId == 0)
				{
					continue;
				}

				FrameworkRuntime::FrameResult::BossGimmickZoneSyncEvent frameEvent{};
				frameEvent.worldId = worldId;
				frameEvent.bossNetId = bossNetId;
				frameEvent.gimmickSeq = zoneEvent.gimmickSeq;
				frameEvent.zoneNetId = zoneEvent.zoneNetId;
				frameEvent.state = static_cast<uint32_t>(zoneEvent.state);
				frameEvent.x = SafeFinite(zoneEvent.position.x);
				frameEvent.y = SafeFinite(zoneEvent.position.y);
				frameEvent.z = SafeFinite(zoneEvent.position.z);
				frameEvent.radius = SafeFinite(zoneEvent.radius);
				outEvents.bossGimmickZones.push_back(frameEvent);
			}

			bossGimmickEvents.objectEvents.clear();
			bossGimmickEvents.zoneEvents.clear();
		}

		// Transfer-imported entities keep the player's existing NetId. They must
		// not pass through the normal spawn auto-allocation path before the server
		// transfer committer rebinds that NetId to the target entity.
		std::unordered_set<Entity> transferImportedEntities;
		for (const WorldLifecycleCommand& command : runtime.LifecycleOutbox())
		{
			if (command.kind == WorldLifecycleCommandKind::TransferImported &&
				!command.entity.IsNull())
			{
				transferImportedEntities.insert(command.entity);
			}
		}

		for (const WorldLifecycleCommand& command : runtime.LifecycleOutbox())
		{
			switch (command.kind) {
			case WorldLifecycleCommandKind::EntitySpawned:
			{
				if (transferImportedEntities.contains(command.entity))
				{
					break;
				}

				if (!view.HasComponent<ReplicatedTag>(command.entity))
				{
					break;
				}

				NetId netId = netIdRegistry.FindNetId(worldId, command.entity);
				if (!netId.IsValid())
				{
					netId = netIdRegistry.Allocate();
					if (!netId.IsValid())
					{
						break;
					}

					if (!netIdRegistry.BindEntity(netId, worldId, command.entity))
					{
						netIdRegistry.Free(netId);
						break;
					}
				}

				FrameworkRuntime::FrameResult::EntitySpawnEvent entitySpawnEvent =
				{
					.worldId = worldId,
					.entity = command.entity,
					.netId = netId
				};
				outEvents.spawns.push_back(entitySpawnEvent);
				break;
			}

			case WorldLifecycleCommandKind::EntityDespawned:
			{
				const NetId netId = netIdRegistry.FindNetId(worldId, command.entity);
				if (!netId.IsValid())
				{
					// A transferred source entity may already have moved its NetId
					// binding to the target entity during the server commit step.
					break;
				}
				
				FrameworkRuntime::FrameResult::EntityDespawnEvent entityDespawnEvent =
				{
					.worldId = worldId,
					.entity = command.entity,
					.netId = netId
				};
				outEvents.despawns.push_back(entityDespawnEvent);

				netIdRegistry.UnbindEntity(netId);
				netIdRegistry.Free(netId);
				break;
			}

			default:
				break;
			}
		}

		runtime.ClearLifecycleOutbox();
	}
}
