#include "pch.h"
#include "ServerPacketStager.h"

#include "FrameworkLog.h"

namespace
{
	constexpr const char* kLogCategory = "ServerPacketStager";

	Protocol::PartyLifecycle ToProtoPartyLifecycle(
		PartyLifecycleState lifecycle) noexcept
	{
		switch (lifecycle)
		{
		case PartyLifecycleState::WorldEntryPending:
			return Protocol::PARTY_LIFECYCLE_WORLD_ENTRY_PENDING;
		case PartyLifecycleState::InWorld:
			return Protocol::PARTY_LIFECYCLE_IN_WORLD;
		case PartyLifecycleState::Disbanded:
			return Protocol::PARTY_LIFECYCLE_DISBANDED;
		case PartyLifecycleState::Forming:
		default:
			return Protocol::PARTY_LIFECYCLE_FORMING;
		}
	}

	Protocol::PartyMemberRole ToProtoPartyMemberRole(
		PartyMemberRole role) noexcept
	{
		return role == PartyMemberRole::Leader
			? Protocol::PARTY_MEMBER_ROLE_LEADER
			: Protocol::PARTY_MEMBER_ROLE_MEMBER;
	}

	Protocol::PartyMemberPresence ToProtoPartyMemberPresence(
		PartyMemberPresence presence) noexcept
	{
		return presence == PartyMemberPresence::Offline
			? Protocol::PARTY_MEMBER_PRESENCE_OFFLINE
			: Protocol::PARTY_MEMBER_PRESENCE_ONLINE;
	}

	Protocol::PartyJoinRequestState ToProtoPartyJoinRequestState(
		PartyJoinRequestState state) noexcept
	{
		switch (state)
		{
		case PartyJoinRequestState::Accepted:
			return Protocol::PARTY_JOIN_REQUEST_ACCEPTED;
		case PartyJoinRequestState::Rejected:
			return Protocol::PARTY_JOIN_REQUEST_REJECTED;
		case PartyJoinRequestState::Cancelled:
			return Protocol::PARTY_JOIN_REQUEST_CANCELLED;
		case PartyJoinRequestState::Expired:
			return Protocol::PARTY_JOIN_REQUEST_EXPIRED;
		case PartyJoinRequestState::Pending:
		default:
			return Protocol::PARTY_JOIN_REQUEST_PENDING;
		}
	}

	Protocol::PartyJoinRequestCloseReason ToProtoPartyJoinRequestCloseReason(
		PartyJoinRequestCloseReason reason) noexcept
	{
		switch (reason)
		{
		case PartyJoinRequestCloseReason::Accepted:
			return Protocol::PARTY_CLOSE_ACCEPTED;
		case PartyJoinRequestCloseReason::RejectedByLeader:
			return Protocol::PARTY_CLOSE_REJECTED_BY_LEADER;
		case PartyJoinRequestCloseReason::CancelledByRequester:
			return Protocol::PARTY_CLOSE_CANCELLED_BY_REQUESTER;
		case PartyJoinRequestCloseReason::Expired:
			return Protocol::PARTY_CLOSE_EXPIRED;
		case PartyJoinRequestCloseReason::ClosedByPartyFull:
			return Protocol::PARTY_CLOSE_PARTY_FULL;
		case PartyJoinRequestCloseReason::ClosedByPartyEnteredWorld:
			return Protocol::PARTY_CLOSE_PARTY_ENTERED_WORLD;
		case PartyJoinRequestCloseReason::ClosedByRequesterJoinedOtherParty:
			return Protocol::PARTY_CLOSE_REQUESTER_JOINED_OTHER_PARTY;
		case PartyJoinRequestCloseReason::ClosedByPartyDisbanded:
			return Protocol::PARTY_CLOSE_PARTY_DISBANDED;
		case PartyJoinRequestCloseReason::None:
		default:
			return Protocol::PARTY_CLOSE_NONE;
		}
	}

	uint32_t ToPartyUiCharacterType(CharacterId characterId) noexcept
	{
		if (characterId == CharacterId::None)
		{
			return 0;
		}

		return static_cast<uint32_t>(characterId) - 1;
	}

	void FillPartyMember(
		Protocol::PartyMember& out,
		const PartyMemberSnapshot& member)
	{
		out.set_sessionid(member.sessionId);
		out.set_accountid(member.accountId);
		out.set_netid(member.netId.GetRaw());
		out.set_charactertype(ToPartyUiCharacterType(member.characterId));
		out.set_role(ToProtoPartyMemberRole(member.role));
		out.set_presence(ToProtoPartyMemberPresence(member.presence));
	}

	void FillPartyJoinRequest(
		Protocol::PartyJoinRequest& out,
		const PartyJoinRequestSnapshot& request)
	{
		out.set_joinrequestid(request.requestId);
		out.set_partyid(request.partyId);
		out.set_requestersessionid(request.requesterSessionId);
		out.set_requesteraccountid(request.requesterAccountId);
		out.set_requestercharactertype(
			ToPartyUiCharacterType(request.requesterCharacterId));
		out.set_state(ToProtoPartyJoinRequestState(request.state));
		out.set_closereason(
			ToProtoPartyJoinRequestCloseReason(request.closeReason));
		out.set_createdatsec(request.createdAtSec);
		out.set_expiresatsec(request.expiresAtSec);
		out.set_closedatsec(request.closedAtSec);
	}

	void FillPartySnapshot(
		Protocol::PartySnapshot& out,
		const PartySnapshot& party)
	{
		out.set_partyid(party.partyId);
		out.set_lifecycle(ToProtoPartyLifecycle(party.lifecycle));
		out.set_leadersessionid(party.leaderSessionId);
		out.set_createdatsec(party.createdAtSec);
		out.set_joinable(party.joinable);

		for (const PartyMemberSnapshot& member : party.members)
		{
			FillPartyMember(*out.add_members(), member);
		}

		for (const PartyJoinRequestSnapshot& request : party.joinRequests)
		{
			FillPartyJoinRequest(*out.add_joinrequests(), request);
		}
	}

	void FillPartyListEntry(
		Protocol::PartyListEntry& out,
		const PartyListEntry& entry)
	{
		out.set_partyid(entry.partyId);
		out.set_leadersessionid(entry.leaderSessionId);
		out.set_leadercharactertype(
			ToPartyUiCharacterType(entry.leaderCharacterId));
		out.set_membercount(entry.memberCount);
		out.set_capacity(entry.capacity);
		out.set_lifecycle(ToProtoPartyLifecycle(entry.lifecycle));
		out.set_createdatsec(entry.createdAtSec);
		out.set_joinable(entry.joinable);
	}

	template<typename TPacket>
	bool StageUnicastPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		PacketType packetType,
		const TPacket& packet)
	{
		SendBuffer* const buffer =
			PacketFactory::Serialize(packetType, packet);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged =
			network.StageUnicast(
				sessionId,
				std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}
}

bool ServerPacketStager::StageLoginSuccess(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId playerNetId)
{
	Protocol::SC_LOGIN_SUCCESS_PACKET loginAck;
	loginAck.set_netid(playerNetId.GetRaw());

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_LOGIN_SUCCESS, loginAck);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));

	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageLoginFail(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t reason)
{
	Protocol::SC_LOGIN_FAIL_PACKET loginFail;
	loginFail.set_reason(reason);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_LOGIN_FAIL, loginFail);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));

	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageSpawnAddPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId netId,
	CharacterId characterId,
	const WorldTransformComp* transform)
{
	Protocol::SC_ADD_PACKET add;
	add.set_netid(netId.GetRaw());
	add.set_typeid_(static_cast<int>(characterId));
	add.set_x(transform != nullptr ? transform->position.x : 0.0f);
	add.set_y(transform != nullptr ? transform->position.y : 0.0f);
	add.set_z(transform != nullptr ? transform->position.z : 0.0f);
	add.set_yaw(transform != nullptr ?
		TransformHelper::QuaternionToYaw(transform->rotation) : 0.0f);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_ADD, add);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageSpawnAddPacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId,
	CharacterId characterId,
	const WorldTransformComp* transform)
{
	if (sessionIds.empty())
	{
		return true;
	}

	Protocol::SC_ADD_PACKET add;
	add.set_netid(netId.GetRaw());
	add.set_typeid_(static_cast<int>(characterId));
	add.set_x(transform != nullptr ? transform->position.x : 0.0f);
	add.set_y(transform != nullptr ? transform->position.y : 0.0f);
	add.set_z(transform != nullptr ? transform->position.z : 0.0f);
	add.set_yaw(transform != nullptr ?
		TransformHelper::QuaternionToYaw(transform->rotation) : 0.0f);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_ADD, add);
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

bool ServerPacketStager::StageSpawnRemovePacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId)
{
	if (sessionIds.empty())
	{
		return true;
	}

	Protocol::SC_REMOVE_PACKET remove;
	remove.set_netid(netId.GetRaw());

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_REMOVE, remove);
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

bool ServerPacketStager::StageWorldTransitionBeginPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	const ServerWorldTransitionBeginPacket& transition)
{
	Protocol::SC_WORLD_TRANSITION_BEGIN_PACKET packet;
	packet.set_transferid(transition.transferId);
	packet.set_requestid(transition.requestId);
	packet.set_sourceworlddefid(transition.sourceWorldDefId);
	packet.set_sourceworldid(transition.sourceWorldId);
	packet.set_targetworlddefid(transition.targetWorldDefId);
	packet.set_targetworldid(transition.targetWorldId);
	packet.set_mapresourceid(transition.mapResourceId);
	packet.set_playernetid(transition.playerNetId);
	packet.set_clearexistingobjects(transition.clearExistingObjects);
	packet.set_waitclientready(transition.waitClientReady);
	packet.set_usedfallback(transition.usedFallback);
	packet.set_reason(transition.reason);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_WORLD_TRANSITION_BEGIN, packet);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageStatPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId netId,
	const CombatStatStateComp& stats)
{
	Protocol::SC_STAT_CHANGE_PACKET statPacket;
	statPacket.set_netid(netId.GetRaw());
	statPacket.set_curhp(
		static_cast<uint32_t>(std::max(0, stats.currentHp)));
	statPacket.set_maxhp(
		static_cast<uint32_t>(std::max(0, stats.maxHp)));
	statPacket.set_curstamina(
		static_cast<uint32_t>(std::max(0, stats.currentStamina)));
	statPacket.set_maxstamina(
		static_cast<uint32_t>(std::max(0, stats.maxStamina)));
	statPacket.set_power(
		static_cast<uint32_t>(std::max(0, stats.attackPower)));
	statPacket.set_attackspeed(stats.attackSpeed);
	statPacket.set_defense(
		static_cast<uint32_t>(std::max(0, stats.defense)));
	statPacket.set_movespeed(std::max(0.0f, stats.moveSpeed));

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_STAT_CHANGE, statPacket);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageStatPacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId,
	const CombatStatStateComp& stats)
{
	if (sessionIds.empty())
	{
		return true;
	}

	Protocol::SC_STAT_CHANGE_PACKET statPacket;
	statPacket.set_netid(netId.GetRaw());
	statPacket.set_curhp(
		static_cast<uint32_t>(std::max(0, stats.currentHp)));
	statPacket.set_maxhp(
		static_cast<uint32_t>(std::max(0, stats.maxHp)));
	statPacket.set_curstamina(
		static_cast<uint32_t>(std::max(0, stats.currentStamina)));
	statPacket.set_maxstamina(
		static_cast<uint32_t>(std::max(0, stats.maxStamina)));
	statPacket.set_power(
		static_cast<uint32_t>(std::max(0, stats.attackPower)));
	statPacket.set_attackspeed(stats.attackSpeed);
	statPacket.set_defense(
		static_cast<uint32_t>(std::max(0, stats.defense)));
	statPacket.set_movespeed(std::max(0.0f, stats.moveSpeed));

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_STAT_CHANGE, statPacket);
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

bool ServerPacketStager::StageAnimationPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId netId,
	const AnimationPlaybackStateComp& playback)
{
	Protocol::SC_ANIMATION_TRANSITION_PACKET animationPacket;
	animationPacket.set_netid(netId.GetRaw());
	animationPacket.set_curranim(static_cast<int32_t>(playback.animationId));
	animationPacket.set_abilityinstanceid(playback.boundAbilityInstanceId);
	animationPacket.set_normalizedtime(playback.normalizedTime);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_ANIMATION_CHANGE,
		animationPacket);
}

bool ServerPacketStager::StageAnimationPacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId,
	const AnimationPlaybackStateComp& playback)
{
	Protocol::SC_ANIMATION_TRANSITION_PACKET animationPacket;
	animationPacket.set_netid(netId.GetRaw());
	animationPacket.set_curranim(static_cast<int32_t>(playback.animationId));
	animationPacket.set_abilityinstanceid(playback.boundAbilityInstanceId);
	animationPacket.set_normalizedtime(playback.normalizedTime);

	return StageReplicationPacket(
		network,
		PacketType::SC_ANIMATION_CHANGE,
		sessionIds,
		animationPacket);
}

bool ServerPacketStager::StageItemCountPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	const ConsumableInventoryComp& inventory)
{
	Protocol::SC_ITEM_COUNT_PACKET itemCountPacket;
	itemCountPacket.set_hppotioncount(inventory.hpPotionCount);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_ITEM_COUNT,
		itemCountPacket);
}

bool ServerPacketStager::StageTeamDeathCountPacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	const PartyDeathCountState& deathCount)
{
	Protocol::SC_TEAM_DEATH_COUNT_PACKET packet;
	packet.set_maxdeathcount(deathCount.initialCount);
	packet.set_deathcount(deathCount.remainingCount);

	return StageReplicationPacket(
		network,
		PacketType::SC_TEAM_DEATH_COUNT,
		sessionIds,
		packet);
}

bool ServerPacketStager::StageMonsterCombatStatePacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId netId,
	bool inCombat)
{
	Protocol::SC_MONSTER_COMBAT_STATE_PACKET packet;
	packet.set_netid(netId.GetRaw());
	packet.set_incombat(inCombat);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_MONSTER_COMBAT_STATE,
		packet);
}

bool ServerPacketStager::StageMonsterCombatStatePacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId,
	bool inCombat)
{
	Protocol::SC_MONSTER_COMBAT_STATE_PACKET packet;
	packet.set_netid(netId.GetRaw());
	packet.set_incombat(inCombat);

	return StageReplicationPacket(
		network,
		PacketType::SC_MONSTER_COMBAT_STATE,
		sessionIds,
		packet);
}

bool ServerPacketStager::StageWorldTransitionRejectedPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t requestId,
	uint32_t reason)
{
	Protocol::SC_WORLD_TRANSITION_REJECTED_PACKET packet;
	packet.set_requestid(requestId);
	packet.set_reason(reason);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_WORLD_TRANSITION_REJECTED, packet);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageTimeSyncPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t probeSeq,
	uint32_t serverSendTimeMs,
	uint64_t serverFrame)
{
	Protocol::SC_TIME_SYNC_PACKET packet;
	packet.set_probeseq(probeSeq);
	packet.set_serversendtimems(serverSendTimeMs);
	packet.set_serverframe(serverFrame);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_TIME_SYNC,
		packet);
}

bool ServerPacketStager::StagePartyUiBootstrapPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t clientRequestId,
	const PartySnapshot* myParty,
	std::span<const PartyListEntry> parties)
{
	FWLOG_INFO(
		kLogCategory,
		"StagePartyUiBootstrap build begin (sid=%u, clientRequestId=%u, hasMyParty=%u, partyCount=%zu)",
		sessionId,
		clientRequestId,
		myParty != nullptr && myParty->partyId != 0 ? 1u : 0u,
		parties.size());

	Protocol::SC_PARTY_UI_BOOTSTRAP_PACKET packet;
	packet.set_clientrequestid(clientRequestId);
	packet.set_hasmyparty(myParty != nullptr && myParty->partyId != 0);
	if (myParty != nullptr && myParty->partyId != 0)
	{
		FillPartySnapshot(*packet.mutable_myparty(), *myParty);
	}

	for (const PartyListEntry& entry : parties)
	{
		FillPartyListEntry(*packet.add_parties(), entry);
	}

	const bool staged = StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_PARTY_UI_BOOTSTRAP,
		packet);
	FWLOG_INFO(
		kLogCategory,
		"StagePartyUiBootstrap stage result (sid=%u, clientRequestId=%u, bodySize=%zu, staged=%u)",
		sessionId,
		clientRequestId,
		packet.ByteSizeLong(),
		staged ? 1u : 0u);
	return staged;
}

bool ServerPacketStager::StagePartyListSnapshotPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t clientRequestId,
	std::span<const PartyListEntry> parties)
{
	Protocol::SC_PARTY_LIST_SNAPSHOT_PACKET packet;
	packet.set_clientrequestid(clientRequestId);
	for (const PartyListEntry& entry : parties)
	{
		FillPartyListEntry(*packet.add_parties(), entry);
	}

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_PARTY_LIST_SNAPSHOT,
		packet);
}

bool ServerPacketStager::StagePartyCommandResultPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t clientRequestId,
	const PartyResult& result)
{
	Protocol::SC_PARTY_COMMAND_RESULT_PACKET packet;
	packet.set_clientrequestid(clientRequestId);
	packet.set_success(result.Succeeded());
	packet.set_error(static_cast<uint32_t>(result.error));
	packet.set_partyid(result.partyId);
	packet.set_joinrequestid(result.requestId);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_PARTY_COMMAND_RESULT,
		packet);
}

bool ServerPacketStager::StagePartySnapshotPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	const PartySnapshot& party)
{
	Protocol::SC_PARTY_SNAPSHOT_PACKET packet;
	FillPartySnapshot(*packet.mutable_party(), party);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_PARTY_SNAPSHOT,
		packet);
}

bool ServerPacketStager::StagePartySnapshotPacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	const PartySnapshot& party)
{
	Protocol::SC_PARTY_SNAPSHOT_PACKET packet;
	FillPartySnapshot(*packet.mutable_party(), party);

	return StageReplicationPacket(
		network,
		PacketType::SC_PARTY_SNAPSHOT,
		sessionIds,
		packet);
}

bool ServerPacketStager::StagePartyJoinRequestReceivedPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	PartyId partyId,
	const PartyJoinRequestSnapshot& request)
{
	Protocol::SC_PARTY_JOIN_REQUEST_RECEIVED_PACKET packet;
	packet.set_partyid(partyId);
	FillPartyJoinRequest(*packet.mutable_request(), request);

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_PARTY_JOIN_REQUEST_RECEIVED,
		packet);
}

bool ServerPacketStager::StagePartyJoinRequestClosedPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	PartyId partyId,
	const PartyJoinRequestSnapshot& request)
{
	Protocol::SC_PARTY_JOIN_REQUEST_CLOSED_PACKET packet;
	packet.set_partyid(partyId);
	packet.set_joinrequestid(request.requestId);
	packet.set_reason(ToProtoPartyJoinRequestCloseReason(request.closeReason));
	packet.set_state(ToProtoPartyJoinRequestState(request.state));

	return StageUnicastPacket(
		network,
		sessionId,
		PacketType::SC_PARTY_JOIN_REQUEST_CLOSED,
		packet);
}
