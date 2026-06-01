#include "pch.h"
#include "ServerFrameEventDispatcher.h"

#include <algorithm>
#include <span>
#include <vector>

#include "BossGimmickReplicationProtocol.h"
#include "FrameworkLog.h"
#include "Protocol.pb.h"
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

	for (const auto& combatImpactEvent : frameResult.events.combatImpacts)
	{
		if (!combatImpactEvent.attackerNetId.IsValid() ||
			!combatImpactEvent.victimNetId.IsValid())
		{
			continue;
		}

		sessionSystem.Flow().CollectSessionsInWorld(
			combatImpactEvent.worldId,
			worldSessionIds);
		RemoveExcludedSessions(worldSessionIds, excludedSessionIds);

		Protocol::SC_COMBAT_IMPACT_PACKET impactPacket;
		impactPacket.set_attackernetid(
			combatImpactEvent.attackerNetId.GetRaw());
		impactPacket.set_victimnetid(
			combatImpactEvent.victimNetId.GetRaw());
		impactPacket.set_resulttype(combatImpactEvent.resultType);
		impactPacket.set_impactx(combatImpactEvent.impactX);
		impactPacket.set_impacty(combatImpactEvent.impactY);
		impactPacket.set_impactz(combatImpactEvent.impactZ);
		impactPacket.set_dirx(combatImpactEvent.dirX);
		impactPacket.set_diry(combatImpactEvent.dirY);
		impactPacket.set_dirz(combatImpactEvent.dirZ);

		(void)ServerPacketStager::StageReplicationPacket(
			sessionSystem.Network(),
			PacketType::SC_COMBAT_IMPACT,
			std::span<const SessionId>(worldSessionIds),
			impactPacket);
	}

	for (const auto& gimmickObjectEvent :
		frameResult.events.bossGimmickObjects)
	{
		if (!gimmickObjectEvent.bossNetId.IsValid() ||
			gimmickObjectEvent.objectNetId == 0)
		{
			continue;
		}

		sessionSystem.Flow().CollectSessionsInWorld(
			gimmickObjectEvent.worldId,
			worldSessionIds);
		RemoveExcludedSessions(worldSessionIds, excludedSessionIds);

		Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET packet;
		BossGimmickReplicationProtocol::FillObjectSyncPacket(
			packet,
			gimmickObjectEvent);

		(void)ServerPacketStager::StageReplicationPacket(
			sessionSystem.Network(),
			PacketType::SC_BOSS_GIMMICK_OBJECT_SYNC,
			std::span<const SessionId>(worldSessionIds),
			packet);
	}

	for (const auto& gimmickZoneEvent :
		frameResult.events.bossGimmickZones)
	{
		if (!gimmickZoneEvent.bossNetId.IsValid() ||
			gimmickZoneEvent.zoneNetId == 0)
		{
			continue;
		}

		sessionSystem.Flow().CollectSessionsInWorld(
			gimmickZoneEvent.worldId,
			worldSessionIds);
		RemoveExcludedSessions(worldSessionIds, excludedSessionIds);

		Protocol::SC_BOSS_GIMMICK_ZONE_SYNC_PACKET packet;
		BossGimmickReplicationProtocol::FillZoneSyncPacket(
			packet,
			gimmickZoneEvent);

		(void)ServerPacketStager::StageReplicationPacket(
			sessionSystem.Network(),
			PacketType::SC_BOSS_GIMMICK_ZONE_SYNC,
			std::span<const SessionId>(worldSessionIds),
			packet);
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
