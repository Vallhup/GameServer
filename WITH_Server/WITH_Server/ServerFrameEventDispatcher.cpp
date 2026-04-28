#include "pch.h"
#include "ServerFrameEventDispatcher.h"

#include <algorithm>
#include <span>
#include <vector>

#include "FrameworkLog.h"
#include "ServerPacketStager.h"
#include "ServerPlayerControlBinding.h"
#include "ServerReplicationSnapshot.h"

namespace
{
	constexpr const char* kLogCategory = "FrameDispatcher";

	void RemoveExcludedSessions(
		std::vector<SessionId>& sessionIds,
		std::span<const SessionId> excludedSessionIds)
	{
		if (excludedSessionIds.empty())
		{
			return;
		}

		sessionIds.erase(
			std::remove_if(
				sessionIds.begin(),
				sessionIds.end(),
				[excludedSessionIds](SessionId sessionId)
				{
					return std::find(
						excludedSessionIds.begin(),
						excludedSessionIds.end(),
						sessionId) != excludedSessionIds.end();
				}),
			sessionIds.end());
	}
}

bool ServerFrameEventDispatcher::Dispatch(
	const FrameworkRuntime::FrameResult& frameResult,
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	SessionBindingRegistry& sessionBindings,
	PlayerEntryService& playerEntryService,
	double nowSec,
	std::span<const SessionId> excludedSessionIds)
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
			RemoveExcludedSessions(worldSessionIds, excludedSessionIds);
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
			FWLOG_ERROR(kLogCategory, "Player control bind failed (sid=%u, worldId=%u, netId=%u)",
				sessionId, spawnEvent.worldId.GetRaw(), spawnEvent.netId.GetRaw());
			return false;
		}

		if (!sessionBindings.Bind(sessionId, spawnEvent.netId, spawnEvent.worldId))
		{
			FWLOG_ERROR(kLogCategory, "Session bind failed (sid=%u, worldId=%u, netId=%u)",
				sessionId, spawnEvent.worldId.GetRaw(), spawnEvent.netId.GetRaw());
			return false;
		}

		if (!framework.AttachPresenceToWorld(sessionId, spawnEvent.worldId, nowSec))
		{
			FWLOG_ERROR(kLogCategory, "Presence attach failed (sid=%u, worldId=%u, netId=%u)",
				sessionId, spawnEvent.worldId.GetRaw(), spawnEvent.netId.GetRaw());
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
			if (worldSessionId != sessionId &&
				std::find(
					excludedSessionIds.begin(),
					excludedSessionIds.end(),
					worldSessionId) == excludedSessionIds.end())
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
		RemoveExcludedSessions(worldSessionIds, excludedSessionIds);
		(void)ServerPacketStager::StageSpawnRemovePacketToSessions(
			network,
			std::span<const SessionId>(worldSessionIds),
			despawnEvent.netId);
	}

	return true;
}
