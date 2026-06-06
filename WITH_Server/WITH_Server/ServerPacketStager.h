#pragma once

#include <cstdint>
#include <span>

#include "CharacterDef.h"
#include "NetworkRuntime.h"
#include "PacketFactory.h"
#include "PartyTypes.h"
#include "Protocol.pb.h"
#include "Session.h"
#include "TitleDef.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "TransformHelper.h"

struct ServerWorldTransitionBeginPacket
{
	uint64_t transferId{ 0 };
	uint32_t requestId{ 0 };
	uint32_t sourceWorldDefId{ 0 };
	uint64_t sourceWorldId{ 0 };
	uint32_t targetWorldDefId{ 0 };
	uint64_t targetWorldId{ 0 };
	uint32_t mapResourceId{ 0 };
	uint64_t playerNetId{ 0 };
	bool clearExistingObjects{ true };
	bool waitClientReady{ true };
	bool usedFallback{ false };
	uint32_t reason{ 0 };
};

class ServerPacketStager final {
public:
	static bool StageLoginSuccess(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId playerNetId);

	static bool StageLoginFail(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t reason);

	static bool StageSpawnAddPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		CharacterId characterId,
		const WorldTransformComp* transform = nullptr);

	static bool StageSpawnAddPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		CharacterId characterId,
		const WorldTransformComp* transform = nullptr);

	static bool StageSpawnRemovePacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId);

	static bool StageWorldTransitionBeginPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		const ServerWorldTransitionBeginPacket& transition);

	static bool StageWorldTransitionRejectedPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t requestId,
		uint32_t reason);

	static bool StageTimeSyncPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t probeSeq,
		uint32_t serverSendTimeMs,
		uint64_t serverFrame);

	static bool StagePartyUiBootstrapPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t clientRequestId,
		const PartySnapshot* myParty,
		std::span<const PartyListEntry> parties);

	static bool StagePartyListSnapshotPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t clientRequestId,
		std::span<const PartyListEntry> parties);

	static bool StagePartyCommandResultPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t clientRequestId,
		const PartyResult& result);

	static bool StagePartySnapshotPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		const PartySnapshot& party);

	static bool StagePartySnapshotPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		const PartySnapshot& party);

	static bool StagePartyJoinRequestReceivedPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		PartyId partyId,
		const PartyJoinRequestSnapshot& request);

	static bool StagePartyJoinRequestClosedPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		PartyId partyId,
		const PartyJoinRequestSnapshot& request);

	static bool StageStatPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		const CombatStatStateComp& stats);

	static bool StageStatPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		const CombatStatStateComp& stats);

	static bool StageStatUiBootstrapPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t clientRequestId,
		std::span<const TitleId> ownedTitleIds,
		TitleId equippedTitleId);

	static bool StageTitleEquipResultPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t clientRequestId,
		bool success,
		TitleId equippedTitleId,
		uint32_t reason);

	static bool StageAnimationPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		const AnimationPlaybackStateComp& playback);

	static bool StageAnimationPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		const AnimationPlaybackStateComp& playback);

	static bool StageItemCountPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		const ConsumableInventoryComp& inventory);

	static bool StageTeamDeathCountPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		const PartyDeathCountState& deathCount);

	static bool StageMonsterCombatStatePacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		bool inCombat);

	static bool StageMonsterCombatStatePacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		bool inCombat);

	template<typename TPacket>
	static bool StageReplicationPacket(
		NetworkRuntime& network,
		PacketType packetType,
		std::span<const SessionId> sessionIds,
		const TPacket& packet)
	{
		if (sessionIds.empty())
		{
			return true;
		}

		SendBuffer* const buffer =
			PacketFactory::Serialize(packetType, packet);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged = network.StageMulticast(
			sessionIds,
			std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}
};
