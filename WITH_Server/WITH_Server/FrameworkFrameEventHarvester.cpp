#include "pch.h"
#include "FrameworkFrameEventHarvester.h"

#include <unordered_set>

#include "NetIdRegistry.h"
#include "RepComponent.h"
#include "WorldInstance.h"
#include "WorldLifecycleCommand.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldRuntime.h"

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
		const ECSView view = runtime.MakeView();

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
