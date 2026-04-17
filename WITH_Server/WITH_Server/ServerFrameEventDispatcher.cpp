#include "pch.h"
#include "ServerFrameEventDispatcher.h"

#include <iostream>
#include <span>
#include <vector>

#include "ServerPacketStager.h"
#include "ServerPlayerControlBinding.h"
#include "ServerReplicationSnapshot.h"

bool ServerFrameEventDispatcher::Dispatch(
	const FrameworkRuntime::FrameResult& frameResult,
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	SessionBindingRegistry& sessionBindings,
	PlayerEntryService& playerEntryService,
	double nowSec)
{
	std::vector<SessionId> worldSessionIds;
	std::vector<SessionId> otherSessionIds;
	for (const auto& spawnEvent : frameResult.events.spawns)
	{
		if (!spawnEvent.netId.IsValid())
		{
			continue;
		}
		CharacterId characterId = CharacterId::None;
		const WorldTransformComp* transform = nullptr;
		if (!ServerReplicationSnapshot::TryGetReplicatedSpawnState(
			framework,
			spawnEvent.worldId,
			spawnEvent.entity,
			characterId,
			transform))
		{
			continue;
		}
		const PendingCharacterSpawn* pendingCharacterSpawn =
			playerEntryService.FindPendingSpawn(spawnEvent.worldId, spawnEvent.entity);
		if (pendingCharacterSpawn == nullptr)
		{
			sessionBindings.CollectSessionsInWorld(spawnEvent.worldId, worldSessionIds);
			(void)ServerPacketStager::StageSpawnAddPacketToSessions(
				network,
				std::span<const SessionId>(worldSessionIds),
				spawnEvent.netId,
				characterId,
				transform);
			continue;
		}
		PendingCharacterSpawn pendingSpawn{};
		if (!playerEntryService.TryConsumeSpawnConfirmed(
			spawnEvent.worldId,
			spawnEvent.entity,
			pendingSpawn))
		{
			continue;
		}
		const SessionId sessionId = pendingSpawn.sessionId;
		if (!ServerPlayerControlBinding::AssignPlayerControlNetId(
			framework,
			spawnEvent.worldId,
			spawnEvent.entity,
			spawnEvent.netId))
		{
			std::cout << "[ServerFrameEventDispatcher] player control bind failed."
				<< " sessionId=" << sessionId
				<< " worldId=" << spawnEvent.worldId.GetRaw()
				<< " netId=" << spawnEvent.netId.GetRaw()
				<< "\n";
			return false;
		}

		if (!sessionBindings.Bind(sessionId, spawnEvent.netId, spawnEvent.worldId))
		{
			std::cout << "[ServerFrameEventDispatcher] session bind failed."
				<< " sessionId=" << sessionId
				<< " worldId=" << spawnEvent.worldId.GetRaw()
				<< " netId=" << spawnEvent.netId.GetRaw()
				<< "\n";
			return false;
		}

		if (!framework.AttachPresenceToWorld(sessionId, spawnEvent.worldId, nowSec))
		{
			std::cout << "[ServerFrameEventDispatcher] presence attach failed."
				<< " sessionId=" << sessionId
				<< " worldId=" << spawnEvent.worldId.GetRaw()
				<< " netId=" << spawnEvent.netId.GetRaw()
				<< "\n";
			return false;
		}

		(void)network.RequestEnterInGame(sessionId, spawnEvent.netId);
		(void)ServerPacketStager::StageLoginResponse(
			network,
			sessionId,
			spawnEvent.netId);
		(void)ServerPacketStager::StageSpawnAddPacketToSession(
			network,
			sessionId,
			spawnEvent.netId,
			characterId,
			transform);
		ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
			framework,
			network,
			spawnEvent.worldId,
			sessionId,
			spawnEvent.netId);
		sessionBindings.CollectSessionsInWorld(spawnEvent.worldId, worldSessionIds);
		otherSessionIds.clear();
		for (SessionId worldSessionId : worldSessionIds)
		{
			if (worldSessionId != sessionId)
			{
				otherSessionIds.push_back(worldSessionId);
			}
		}
		(void)ServerPacketStager::StageSpawnAddPacketToSessions(
			network,
			std::span<const SessionId>(otherSessionIds),
			spawnEvent.netId,
			characterId,
			transform);
	}

	for (const auto& despawnEvent : frameResult.events.despawns)
	{
		if (!despawnEvent.netId.IsValid())
		{
			continue;
		}

		sessionBindings.CollectSessionsInWorld(
			despawnEvent.worldId,
			worldSessionIds);
		(void)ServerPacketStager::StageSpawnRemovePacketToSessions(
			network,
			std::span<const SessionId>(worldSessionIds),
			despawnEvent.netId);
	}

	return true;
}
