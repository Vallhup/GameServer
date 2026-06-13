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

	static bool StageBeaconCinematicStartPacket(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		uint64_t cinematicInstanceId,
		uint32_t clientRequestId,
		uint64_t partyId,
		uint64_t sourceWorldId,
		uint32_t cinematicType,
		uint64_t initiatorNetId);

	// FinalBoss 처치 후 파티 PvP/엔딩 선택 UI를 띄우라는 신호.
	static bool StageFinalClearChoiceBeginPacket(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		uint64_t voteId,
		uint64_t partyId,
		uint64_t sourceWorldId,
		uint32_t timeoutMs,
		uint64_t deadlineServerMs,
		uint32_t eligibleCount);

	// 투표 결과(PvP행 / 엔딩 후 Plaza행) 통지.
	static bool StageFinalClearChoiceResultPacket(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		uint64_t voteId,
		uint32_t outcome,
		uint32_t reason,
		uint32_t targetWorldDefId,
		uint64_t pvpChooserNetId);

	// PvP 라운드 결과(라스트맨) 통지 → 클라 페이드/엔딩 연출 트리거.
	static bool StagePvpRoundResultPacket(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		uint64_t winnerNetId);

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

	static bool StageGameplayEffectPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		const GameplayEffectStateComp& effects,
		const GameplayEffectReplicationComp& replication,
		bool includeAppliedEvents);

	static bool StageGameplayEffectPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		const GameplayEffectStateComp& effects,
		const GameplayEffectReplicationComp& replication,
		bool includeAppliedEvents);

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

	// 한 플레이어가 장착 중인 칭호를 같은 월드(Plaza)의 모든 세션에
	// 브로드캐스트한다. ownerNetId 본인 세션도 sessionIds에 포함되어야 한다.
	static bool StageTitleReplicationPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId ownerNetId,
		TitleId titleId);

	static bool StageTitleReplicationPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId ownerNetId,
		TitleId titleId);

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
