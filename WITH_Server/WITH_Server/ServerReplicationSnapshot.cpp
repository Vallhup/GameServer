#include "pch.h"
#include "ServerReplicationSnapshot.h"

#include <algorithm>

#include "RepComponent.h"
#include "ServerPacketStager.h"
#include "WorldInstance.h"

namespace
{
	bool IsExcludedNetId(
		NetId netId,
		std::span<const NetId> excludedNetIds) noexcept
	{
		return std::find(
			excludedNetIds.begin(),
			excludedNetIds.end(),
			netId) != excludedNetIds.end();
	}
}

bool ServerReplicationSnapshot::TryGetReplicatedSpawnState(
	FrameworkRuntime& framework,
	WorldId worldId,
	Entity entity,
	CharacterId& outCharacterId,
	const WorldTransformComp*& outTransform)
{
	WorldInstance* const world = framework.FindWorld(worldId);
	if (world == nullptr)
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	if (!view.HasComponent<ReplicatedTag>(entity))
	{
		return false;
	}

	const SpawnTypeComp* const spawnType =
		view.GetComponent<SpawnTypeComp>(entity);
	if (spawnType == nullptr)
	{
		return false;
	}

	outCharacterId = spawnType->characterId;
	outTransform = view.GetComponent<WorldTransformComp>(entity);
	return true;
}

void ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	WorldId worldId,
	SessionId sessionId,
	NetId excludedNetId)
{
	if (!excludedNetId.IsValid())
	{
		StageExistingWorldEntitiesForSession(
			framework,
			network,
			worldId,
			sessionId,
			std::span<const NetId>());
		return;
	}

	const NetId excludedNetIds[] = { excludedNetId };
	StageExistingWorldEntitiesForSession(
		framework,
		network,
		worldId,
		sessionId,
		std::span<const NetId>(excludedNetIds, 1));
}

void ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	WorldId worldId,
	SessionId sessionId,
	std::span<const NetId> excludedNetIds)
{
	WorldInstance* const world = framework.FindWorld(worldId);
	if (world == nullptr)
	{
		return;
	}

	ECSView view = world->GetRuntime().MakeView();
	for (auto [entity, spawnType] : view.View<SpawnTypeComp>())
	{
		if (!view.HasComponent<ReplicatedTag>(entity))
		{
			continue;
		}

		const NetId entityNetId = framework.FindNetId(worldId, entity);
		if (!entityNetId.IsValid() ||
			IsExcludedNetId(entityNetId, excludedNetIds))
		{
			continue;
		}

		const WorldTransformComp* const transform =
			view.GetComponent<WorldTransformComp>(entity);
		(void)ServerPacketStager::StageSpawnAddPacketToSession(
			network,
			sessionId,
			entityNetId,
			spawnType.characterId,
			transform);
	}
}
