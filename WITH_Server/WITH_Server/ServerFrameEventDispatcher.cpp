#include "pch.h"
#include "ServerFrameEventDispatcher.h"

#include <algorithm>
#include <span>
#include <vector>

#include "FrameworkLog.h"
#include "SessionFlowCommands.h"
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
	ServerSessionSystem& sessionSystem,
	double nowSec,
	std::span<const SessionId> excludedSessionIds)
{
	std::vector<SessionId> worldSessionIds;
	(void)nowSec;
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
		const PendingSessionCharacterSpawn* pendingCharacterSpawn =
			sessionSystem.CharacterSpawn().FindPendingSpawn(spawnEvent.worldId, spawnEvent.entity);
		if (pendingCharacterSpawn == nullptr)
		{
			sessionSystem.Flow().CollectSessionsInWorld(spawnEvent.worldId, worldSessionIds);
			RemoveExcludedSessions(worldSessionIds, excludedSessionIds);
			(void)ServerPacketStager::StageSpawnAddPacketToSessions(
				sessionSystem.Network(),
				std::span<const SessionId>(worldSessionIds),
				spawnEvent.netId,
				characterId,
				transform);
			continue;
		}
		PendingSessionCharacterSpawn pendingSpawn{};
		if (!sessionSystem.CharacterSpawn().TryConsumeSpawnConfirmed(
			spawnEvent.worldId,
			spawnEvent.entity,
			pendingSpawn))
		{
			continue;
		}
		const SessionId sessionId = pendingSpawn.sessionId;
		CharacterSpawnConfirmed spawnConfirmed{};
		spawnConfirmed.worldId = spawnEvent.worldId;
		spawnConfirmed.entity = spawnEvent.entity;
		spawnConfirmed.netId = spawnEvent.netId;
		spawnConfirmed.characterId = characterId;
		const SessionFlowResult flowResult =
			sessionSystem.Flow().Dispatch(sessionId, spawnConfirmed);
		if (!flowResult.Succeeded())
		{
			FWLOG_ERROR(kLogCategory, "Spawn confirmed rejected by session flow (sid=%u, worldId=%u, netId=%u)",
				sessionId, spawnEvent.worldId.GetRaw(), spawnEvent.netId.GetRaw());
			return false;
		}

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

		if (!sessionSystem.BeginInitialWorldEntry(
			sessionId,
			spawnEvent.worldId,
			spawnEvent.entity,
			spawnEvent.netId,
			characterId))
		{
			FWLOG_ERROR(kLogCategory, "Initial world entry begin failed (sid=%u, worldId=%u, netId=%u)",
				sessionId, spawnEvent.worldId.GetRaw(), spawnEvent.netId.GetRaw());
			return false;
		}
	}

	for (const auto& despawnEvent : frameResult.events.despawns)
	{
		if (!despawnEvent.netId.IsValid())
		{
			continue;
		}

		sessionSystem.Flow().CollectSessionsInWorld(
			despawnEvent.worldId,
			worldSessionIds);
		RemoveExcludedSessions(worldSessionIds, excludedSessionIds);
		(void)ServerPacketStager::StageSpawnRemovePacketToSessions(
			sessionSystem.Network(),
			std::span<const SessionId>(worldSessionIds),
			despawnEvent.netId);
	}

	return true;
}
