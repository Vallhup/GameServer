#include "pch.h"
#include "PartyService.h"

#include <algorithm>
#include <cassert>
#include <thread>

PartyService::PartyService(IPartySessionQuery& sessionQuery)
	: _sessionQuery(sessionQuery)
{
}

void PartyService::Clear() noexcept
{
	AssertOwnerThread();

	_parties.clear();
	_partyBySession.clear();
	_partyByRequest.clear();
	_nextPartyId = 1;
	_nextRequestId = 1;
}

void PartyService::BindOwnerThreadForDebug() noexcept
{
	_ownerThreadId = std::this_thread::get_id();
}

PartyResult PartyService::CreateParty(
	SessionId leaderSessionId,
	double nowSec)
{
	AssertOwnerThread();

	const SessionId members[] = { leaderSessionId };
	return CreatePartyInternal(
		leaderSessionId,
		std::span<const SessionId>(members, 1),
		PartyFormationSource::ExplicitCreate,
		nowSec);
}

PartyResult PartyService::CreatePartyFromTrustedMembers(
	SessionId leaderSessionId,
	std::span<const SessionId> memberSessionIds,
	PartyFormationSource source,
	double nowSec)
{
	AssertOwnerThread();

	if (source != PartyFormationSource::DemoAutoWorldTransition &&
		source != PartyFormationSource::RestoredFromDB)
	{
		return PartyResult{ .error = PartyError::InvalidPartyState };
	}

	return CreatePartyInternal(
		leaderSessionId,
		memberSessionIds,
		source,
		nowSec);
}

PartyResult PartyService::RequestJoin(
	SessionId requesterSessionId,
	PartyId partyId,
	double nowSec)
{
	AssertOwnerThread();
	ExpireJoinRequests(nowSec);

	if (requesterSessionId == 0 ||
		!_sessionQuery.IsPartyEligible(requesterSessionId))
	{
		return PartyResult{ .error = PartyError::InvalidSession, .partyId = partyId };
	}

	if (FindPartyBySession(requesterSessionId) != 0)
	{
		return PartyResult{ .error = PartyError::AlreadyInParty, .partyId = partyId };
	}

	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyResult{ .error = PartyError::PartyNotFound, .partyId = partyId };
	}

	if (party->lifecycle != PartyLifecycleState::Forming)
	{
		return PartyResult{ .error = PartyError::InvalidPartyState, .partyId = partyId };
	}

	if (party->members.size() >= MaxPartyMembers)
	{
		return PartyResult{ .error = PartyError::PartyFull, .partyId = partyId };
	}

	if (IsRejectCooldownActive(*party, requesterSessionId, nowSec))
	{
		return PartyResult{ .error = PartyError::RequestCooldown, .partyId = partyId };
	}

	if (HasPendingJoinRequest(*party, requesterSessionId))
	{
		return PartyResult{ .error = PartyError::AlreadyRequested, .partyId = partyId };
	}

	const PartyRequestId requestId = AllocateRequestId();
	party->joinRequests.push_back(PartyJoinRequest{
		.requestId = requestId,
		.partyId = partyId,
		.requesterSessionId = requesterSessionId,
		.requesterAccountId = _sessionQuery.FindAccountId(requesterSessionId),
		.requesterCharacterId =
			_sessionQuery.FindSelectedCharacterId(requesterSessionId),
		.state = PartyJoinRequestState::Pending,
		.createdAtSec = nowSec,
		.expiresAtSec = nowSec + PartyJoinRequestTimeoutSec
	});
	_partyByRequest[requestId] = partyId;

	return PartyResult{
		.error = PartyError::None,
		.partyId = partyId,
		.requestId = requestId
	};
}

PartyResult PartyService::AcceptJoinRequest(
	SessionId leaderSessionId,
	PartyRequestId requestId,
	double nowSec)
{
	AssertOwnerThread();
	ExpireJoinRequests(nowSec);

	const auto partyIt = _partyByRequest.find(requestId);
	if (partyIt == _partyByRequest.end())
	{
		return PartyResult{ .error = PartyError::RequestNotFound, .requestId = requestId };
	}

	PartyRecord* const party = FindParty(partyIt->second);
	if (party == nullptr)
	{
		return PartyResult{
			.error = PartyError::PartyNotFound,
			.partyId = partyIt->second,
			.requestId = requestId
		};
	}

	if (party->leaderSessionId != leaderSessionId)
	{
		return PartyResult{
			.error = PartyError::NotPartyLeader,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	if (party->lifecycle != PartyLifecycleState::Forming)
	{
		return PartyResult{
			.error = PartyError::InvalidPartyState,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	auto requestIt = std::find_if(
		party->joinRequests.begin(),
		party->joinRequests.end(),
		[requestId](const PartyJoinRequest& request)
		{
			return request.requestId == requestId;
		});
	if (requestIt == party->joinRequests.end())
	{
		return PartyResult{
			.error = PartyError::RequestNotFound,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	if (requestIt->state != PartyJoinRequestState::Pending)
	{
		return PartyResult{
			.error = PartyError::RequestNotPending,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	const SessionId requesterSessionId = requestIt->requesterSessionId;
	if (FindPartyBySession(requesterSessionId) != 0 ||
		!_sessionQuery.IsPartyEligible(requesterSessionId))
	{
		return PartyResult{
			.error = PartyError::RequesterUnavailable,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	if (party->members.size() >= MaxPartyMembers)
	{
		return PartyResult{
			.error = PartyError::PartyFull,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	CloseJoinRequest(
		*requestIt,
		PartyJoinRequestState::Accepted,
		PartyJoinRequestCloseReason::Accepted,
		nowSec);
	party->members.push_back(PartyMember{
		.sessionId = requesterSessionId,
		.accountId = _sessionQuery.FindAccountId(requesterSessionId),
		.netId = _sessionQuery.FindControlledNetId(requesterSessionId),
		.characterId = _sessionQuery.FindSelectedCharacterId(requesterSessionId),
		.role = PartyMemberRole::Member,
		.presence = PartyMemberPresence::Online,
		.joinedAtSec = nowSec,
		.lastSeenAtSec = nowSec
	});
	_partyBySession[requesterSessionId] = party->partyId;
	ClosePendingRequestsByRequester(
		requesterSessionId,
		party->partyId,
		PartyJoinRequestCloseReason::ClosedByRequesterJoinedOtherParty,
		nowSec);
	if (party->members.size() >= MaxPartyMembers)
	{
		ClosePendingRequestsForParty(
			*party,
			PartyJoinRequestCloseReason::ClosedByPartyFull,
			nowSec);
	}

	return PartyResult{
		.error = PartyError::None,
		.partyId = party->partyId,
		.requestId = requestId
	};
}

PartyResult PartyService::RejectJoinRequest(
	SessionId leaderSessionId,
	PartyRequestId requestId,
	double nowSec)
{
	AssertOwnerThread();
	ExpireJoinRequests(nowSec);

	const auto partyIt = _partyByRequest.find(requestId);
	if (partyIt == _partyByRequest.end())
	{
		return PartyResult{ .error = PartyError::RequestNotFound, .requestId = requestId };
	}

	PartyRecord* const party = FindParty(partyIt->second);
	if (party == nullptr)
	{
		return PartyResult{
			.error = PartyError::PartyNotFound,
			.partyId = partyIt->second,
			.requestId = requestId
		};
	}

	if (party->leaderSessionId != leaderSessionId)
	{
		return PartyResult{
			.error = PartyError::NotPartyLeader,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	auto requestIt = std::find_if(
		party->joinRequests.begin(),
		party->joinRequests.end(),
		[requestId](const PartyJoinRequest& request)
		{
			return request.requestId == requestId;
		});
	if (requestIt == party->joinRequests.end())
	{
		return PartyResult{
			.error = PartyError::RequestNotFound,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	if (requestIt->state != PartyJoinRequestState::Pending)
	{
		return PartyResult{
			.error = PartyError::RequestNotPending,
			.partyId = party->partyId,
			.requestId = requestId
		};
	}

	CloseJoinRequest(
		*requestIt,
		PartyJoinRequestState::Rejected,
		PartyJoinRequestCloseReason::RejectedByLeader,
		nowSec);
	return PartyResult{
		.error = PartyError::None,
		.partyId = party->partyId,
		.requestId = requestId
	};
}

PartyResult PartyService::MarkMemberPresence(
	SessionId memberSessionId,
	PartyMemberPresence presence,
	double nowSec)
{
	AssertOwnerThread();

	const PartyId partyId = FindPartyBySession(memberSessionId);
	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyResult{ .error = PartyError::PartyNotFound, .partyId = partyId };
	}

	auto memberIt = std::find_if(
		party->members.begin(),
		party->members.end(),
		[memberSessionId](const PartyMember& member)
		{
			return member.sessionId == memberSessionId;
		});
	if (memberIt == party->members.end())
	{
		return PartyResult{ .error = PartyError::InvalidSession, .partyId = partyId };
	}

	memberIt->presence = presence;
	if (presence == PartyMemberPresence::Online)
	{
		memberIt->lastSeenAtSec = nowSec;
	}

	ReassignLeaderAfterPresenceChange(*party);

	const bool anyOnline = std::any_of(
		party->members.begin(),
		party->members.end(),
		[](const PartyMember& member)
		{
			return member.presence == PartyMemberPresence::Online;
		});
	if (!anyOnline)
	{
		DisbandParty(*party, nowSec);
	}

	return PartyResult{ .error = PartyError::None, .partyId = partyId };
}

void PartyService::ExpireJoinRequests(double nowSec)
{
	AssertOwnerThread();

	for (auto& [partyId, party] : _parties)
	{
		(void)partyId;
		for (PartyJoinRequest& request : party.joinRequests)
		{
			if (request.state == PartyJoinRequestState::Pending &&
				request.expiresAtSec > 0.0 &&
				request.expiresAtSec <= nowSec)
			{
				CloseJoinRequest(
					request,
					PartyJoinRequestState::Expired,
					PartyJoinRequestCloseReason::Expired,
					nowSec);
			}
		}
	}
}

PartyWorldEntryResult PartyService::BeginWorldEntry(
	SessionId leaderSessionId,
	const WorldTargetSpec& target,
	double nowSec,
	bool allowFallback)
{
	AssertOwnerThread();
	ExpireJoinRequests(nowSec);

	const PartyId partyId = FindPartyBySession(leaderSessionId);
	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyWorldEntryResult{ .error = PartyError::PartyNotFound, .partyId = partyId };
	}

	if (party->leaderSessionId != leaderSessionId)
	{
		return PartyWorldEntryResult{ .error = PartyError::NotPartyLeader, .partyId = partyId };
	}

	if (party->lifecycle == PartyLifecycleState::WorldEntryPending ||
		party->worldEntry.state == PartyWorldEntryState::Requested ||
		party->worldEntry.state == PartyWorldEntryState::TransferEnqueued)
	{
		return PartyWorldEntryResult{
			.error = PartyError::WorldEntryAlreadyPending,
			.partyId = partyId
		};
	}

	if (party->lifecycle == PartyLifecycleState::Disbanded)
	{
		return PartyWorldEntryResult{ .error = PartyError::InvalidPartyState, .partyId = partyId };
	}

	if (!target.explicitTargetId.has_value() &&
		!target.targetWorldDefId.has_value())
	{
		return PartyWorldEntryResult{ .error = PartyError::InvalidTarget, .partyId = partyId };
	}

	const WorldId sourceWorldId = _sessionQuery.FindCurrentWorldId(leaderSessionId);
	if (!sourceWorldId.IsValid() ||
		!_sessionQuery.CanBeginWorldTransfer(leaderSessionId))
	{
		return PartyWorldEntryResult{ .error = PartyError::LeaderUnavailable, .partyId = partyId };
	}

	for (const PartyMember& member : party->members)
	{
		if (member.sessionId == 0 ||
			member.presence != PartyMemberPresence::Online ||
			!_sessionQuery.CanBeginWorldTransfer(member.sessionId))
		{
			return PartyWorldEntryResult{ .error = PartyError::MemberUnavailable, .partyId = partyId };
		}

		if (_sessionQuery.FindCurrentWorldId(member.sessionId) != sourceWorldId)
		{
			return PartyWorldEntryResult{ .error = PartyError::SourceWorldMismatch, .partyId = partyId };
		}
	}

	std::vector<SessionId> snapshot = BuildMemberSessionSnapshot(*party);
	if (snapshot.empty())
	{
		return PartyWorldEntryResult{ .error = PartyError::MemberUnavailable, .partyId = partyId };
	}

	party->lifecycle = PartyLifecycleState::WorldEntryPending;
	ClosePendingRequestsForParty(
		*party,
		PartyJoinRequestCloseReason::ClosedByPartyEnteredWorld,
		nowSec);
	party->worldEntry = PartyWorldEntry{
		.state = PartyWorldEntryState::Requested,
		.transferId = 0,
		.sourceWorldId = sourceWorldId,
		.target = target,
		.sessionSnapshot = snapshot,
		.requestedAtSec = nowSec,
		.completedAtSec = 0.0
	};

	WorldTransferRequest request{};
	request.sessionIds = std::move(snapshot);
	request.sourceWorldId = sourceWorldId;
	request.target = target;
	request.partyId = partyId;
	request.allowFallback = allowFallback;
	request.createdAtSec = nowSec;

	return PartyWorldEntryResult{
		.error = PartyError::None,
		.partyId = partyId,
		.request = std::move(request)
	};
}

PartyResult PartyService::MarkWorldEntryEnqueued(
	PartyId partyId,
	TransferId transferId,
	double /*nowSec*/)
{
	AssertOwnerThread();

	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyResult{ .error = PartyError::PartyNotFound, .partyId = partyId };
	}

	if (transferId == 0)
	{
		return PartyResult{ .error = PartyError::TransferRejected, .partyId = partyId };
	}

	if (party->lifecycle != PartyLifecycleState::WorldEntryPending ||
		party->worldEntry.state != PartyWorldEntryState::Requested)
	{
		return PartyResult{ .error = PartyError::InvalidPartyState, .partyId = partyId };
	}

	party->worldEntry.state = PartyWorldEntryState::TransferEnqueued;
	party->worldEntry.transferId = transferId;

	return PartyResult{ .error = PartyError::None, .partyId = partyId };
}

PartyResult PartyService::CompleteWorldEntry(
	PartyId partyId,
	TransferId transferId,
	WorldId targetWorldId,
	double nowSec)
{
	AssertOwnerThread();

	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyResult{ .error = PartyError::PartyNotFound, .partyId = partyId };
	}

	if (party->lifecycle != PartyLifecycleState::WorldEntryPending ||
		party->worldEntry.state != PartyWorldEntryState::TransferEnqueued ||
		party->worldEntry.transferId != transferId ||
		!targetWorldId.IsValid())
	{
		return PartyResult{ .error = PartyError::InvalidPartyState, .partyId = partyId };
	}

	party->lifecycle = PartyLifecycleState::InWorld;
	party->worldEntry.state = PartyWorldEntryState::Completed;
	party->worldEntry.completedAtSec = nowSec;

	return PartyResult{ .error = PartyError::None, .partyId = partyId };
}

PartyResult PartyService::FailWorldEntry(
	PartyId partyId,
	TransferId transferId,
	double nowSec)
{
	AssertOwnerThread();

	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyResult{ .error = PartyError::PartyNotFound, .partyId = partyId };
	}

	const bool requestedFailure =
		party->worldEntry.state == PartyWorldEntryState::Requested &&
		transferId == 0;
	const bool enqueuedFailure =
		party->worldEntry.state == PartyWorldEntryState::TransferEnqueued &&
		party->worldEntry.transferId == transferId;
	if (party->lifecycle != PartyLifecycleState::WorldEntryPending ||
		(!requestedFailure && !enqueuedFailure))
	{
		return PartyResult{ .error = PartyError::InvalidPartyState, .partyId = partyId };
	}

	party->lifecycle = PartyLifecycleState::Forming;
	party->worldEntry.state = PartyWorldEntryState::Failed;
	party->worldEntry.completedAtSec = nowSec;

	return PartyResult{ .error = PartyError::None, .partyId = partyId };
}

PartyDeathCountResult PartyService::InitializeDeathCountForRun(
	PartyId partyId,
	double nowSec)
{
	AssertOwnerThread();

	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyDeathCountResult{
			.error = PartyError::PartyNotFound,
			.partyId = partyId
		};
	}

	const uint32_t memberCount =
		static_cast<uint32_t>(party->members.size());
	const uint32_t initialCount = memberCount * DeathCountPerPartyMember;
	const uint64_t nextRevision = party->deathCount.revision + 1;

	party->deathCount = PartyDeathCountState{
		.initialCount = initialCount,
		.remainingCount = initialCount,
		.revision = nextRevision,
		.initializedAtSec = nowSec,
		.updatedAtSec = nowSec,
		.initialized = true,
		.exhausted = initialCount == 0
	};

	return BuildDeathCountResult(*party);
}

PartyDeathCountResult PartyService::ConsumeDeathCount(
	PartyId partyId,
	SessionId deadSessionId,
	double nowSec)
{
	AssertOwnerThread();

	PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return PartyDeathCountResult{
			.error = PartyError::PartyNotFound,
			.partyId = partyId,
			.consumedBySessionId = deadSessionId
		};
	}

	if (!party->deathCount.initialized)
	{
		return BuildDeathCountResult(
			*party,
			PartyError::InvalidPartyState,
			deadSessionId);
	}

	if (deadSessionId == 0 || !HasMemberSession(*party, deadSessionId))
	{
		return BuildDeathCountResult(
			*party,
			PartyError::InvalidSession,
			deadSessionId);
	}

	bool consumed = false;
	if (party->deathCount.remainingCount > 0)
	{
		--party->deathCount.remainingCount;
		party->deathCount.updatedAtSec = nowSec;
		++party->deathCount.revision;
		consumed = true;
	}

	party->deathCount.exhausted =
		party->deathCount.remainingCount == 0;

	return BuildDeathCountResult(
		*party,
		PartyError::None,
		deadSessionId,
		consumed);
}

const PartyRecord* PartyService::FindParty(PartyId partyId) const noexcept
{
	const auto it = _parties.find(partyId);
	return it != _parties.end() ? &it->second : nullptr;
}

PartyRecord* PartyService::FindParty(PartyId partyId) noexcept
{
	const auto it = _parties.find(partyId);
	return it != _parties.end() ? &it->second : nullptr;
}

PartyId PartyService::FindPartyBySession(SessionId sessionId) const noexcept
{
	const auto it = _partyBySession.find(sessionId);
	return it != _partyBySession.end() ? it->second : 0;
}

PartyId PartyService::FindPartyByRequest(PartyRequestId requestId) const noexcept
{
	const auto it = _partyByRequest.find(requestId);
	return it != _partyByRequest.end() ? it->second : 0;
}

const PartyJoinRequest* PartyService::FindJoinRequest(
	PartyRequestId requestId) const noexcept
{
	const PartyRecord* const party = FindParty(FindPartyByRequest(requestId));
	if (party == nullptr)
	{
		return nullptr;
	}

	const auto requestIt = std::find_if(
		party->joinRequests.begin(),
		party->joinRequests.end(),
		[requestId](const PartyJoinRequest& request)
		{
			return request.requestId == requestId;
		});
	return requestIt != party->joinRequests.end() ? &*requestIt : nullptr;
}

SessionId PartyService::FindLeaderSession(PartyId partyId) const noexcept
{
	const PartyRecord* const party = FindParty(partyId);
	return party != nullptr ? party->leaderSessionId : 0;
}

PartySnapshot PartyService::BuildPartySnapshot(PartyId partyId) const
{
	PartySnapshot snapshot{};
	const PartyRecord* const party = FindParty(partyId);
	if (party == nullptr)
	{
		return snapshot;
	}

	snapshot.partyId = party->partyId;
	snapshot.formationSource = party->formationSource;
	snapshot.lifecycle = party->lifecycle;
	snapshot.leaderSessionId = party->leaderSessionId;
	snapshot.worldEntry = party->worldEntry;
	snapshot.createdAtSec = party->createdAtSec;
	snapshot.joinable =
		party->lifecycle == PartyLifecycleState::Forming &&
		party->members.size() < MaxPartyMembers;

	snapshot.members.reserve(party->members.size());
	for (const PartyMember& member : party->members)
	{
		snapshot.members.push_back(PartyMemberSnapshot{
			.sessionId = member.sessionId,
			.accountId = member.accountId,
			.netId = member.netId,
			.characterId = member.characterId,
			.role = member.role,
			.presence = member.presence,
			.joinedAtSec = member.joinedAtSec,
			.lastSeenAtSec = member.lastSeenAtSec
		});
	}

	snapshot.joinRequests.reserve(party->joinRequests.size());
	for (const PartyJoinRequest& request : party->joinRequests)
	{
		snapshot.joinRequests.push_back(PartyJoinRequestSnapshot{
			.requestId = request.requestId,
			.partyId = request.partyId,
			.requesterSessionId = request.requesterSessionId,
			.requesterAccountId = request.requesterAccountId,
			.requesterCharacterId = request.requesterCharacterId,
			.state = request.state,
			.closeReason = request.closeReason,
			.createdAtSec = request.createdAtSec,
			.expiresAtSec = request.expiresAtSec,
			.closedAtSec = request.closedAtSec
		});
	}

	return snapshot;
}

PartySnapshot PartyService::BuildPartySnapshotForSession(SessionId sessionId) const
{
	return BuildPartySnapshot(FindPartyBySession(sessionId));
}

PartyDeathCountState PartyService::GetDeathCountSnapshot(
	PartyId partyId) const noexcept
{
	const PartyRecord* const party = FindParty(partyId);
	return party != nullptr ? party->deathCount : PartyDeathCountState{};
}

void PartyService::CollectPublicPartyList(
	std::vector<PartyListEntry>& outEntries,
	size_t limit) const
{
	outEntries.clear();
	for (const auto& [partyId, party] : _parties)
	{
		(void)partyId;
		if (party.lifecycle != PartyLifecycleState::Forming)
		{
			continue;
		}

		outEntries.push_back(PartyListEntry{
			.partyId = party.partyId,
			.leaderSessionId = party.leaderSessionId,
			.leaderCharacterId = CharacterId::None,
			.memberCount = static_cast<uint32_t>(party.members.size()),
			.capacity = MaxPartyMembers,
			.lifecycle = party.lifecycle,
			.createdAtSec = party.createdAtSec,
			.joinable = party.members.size() < MaxPartyMembers
		});
		for (const PartyMember& member : party.members)
		{
			if (member.sessionId == party.leaderSessionId)
			{
				outEntries.back().leaderCharacterId = member.characterId;
				break;
			}
		}
	}

	std::sort(
		outEntries.begin(),
		outEntries.end(),
		[](const PartyListEntry& lhs, const PartyListEntry& rhs)
		{
			if (lhs.createdAtSec != rhs.createdAtSec)
			{
				return lhs.createdAtSec > rhs.createdAtSec;
			}
			return lhs.partyId > rhs.partyId;
		});

	if (outEntries.size() > limit)
	{
		outEntries.resize(limit);
	}
}

PartyResult PartyService::CreatePartyInternal(
	SessionId leaderSessionId,
	std::span<const SessionId> memberSessionIds,
	PartyFormationSource source,
	double nowSec)
{
	if (leaderSessionId == 0 ||
		memberSessionIds.empty() ||
		memberSessionIds.size() > MaxPartyMembers)
	{
		return PartyResult{ .error = PartyError::InvalidSession };
	}

	std::vector<SessionId> uniqueMembers;
	uniqueMembers.reserve(memberSessionIds.size());
	for (SessionId sessionId : memberSessionIds)
	{
		if (sessionId == 0 ||
			!_sessionQuery.IsPartyEligible(sessionId))
		{
			return PartyResult{ .error = PartyError::InvalidSession };
		}

		if (FindPartyBySession(sessionId) != 0)
		{
			return PartyResult{ .error = PartyError::AlreadyInParty };
		}

		if (std::find(uniqueMembers.begin(), uniqueMembers.end(), sessionId) !=
			uniqueMembers.end())
		{
			return PartyResult{ .error = PartyError::DuplicateMember };
		}

		uniqueMembers.push_back(sessionId);
	}

	if (std::find(uniqueMembers.begin(), uniqueMembers.end(), leaderSessionId) ==
		uniqueMembers.end())
	{
		return PartyResult{ .error = PartyError::LeaderUnavailable };
	}

	std::sort(uniqueMembers.begin(), uniqueMembers.end());

	const PartyId partyId = AllocatePartyId();
	PartyRecord party{};
	party.partyId = partyId;
	party.formationSource = source;
	party.lifecycle = PartyLifecycleState::Forming;
	party.leaderSessionId = leaderSessionId;
	party.createdAtSec = nowSec;
	party.members.reserve(uniqueMembers.size());

	for (SessionId sessionId : uniqueMembers)
	{
		party.members.push_back(PartyMember{
			.sessionId = sessionId,
			.accountId = _sessionQuery.FindAccountId(sessionId),
			.netId = _sessionQuery.FindControlledNetId(sessionId),
			.characterId = _sessionQuery.FindSelectedCharacterId(sessionId),
			.role = sessionId == leaderSessionId
				? PartyMemberRole::Leader
				: PartyMemberRole::Member,
			.presence = PartyMemberPresence::Online,
			.joinedAtSec = nowSec,
			.lastSeenAtSec = nowSec
		});
		_partyBySession[sessionId] = partyId;
	}

	if (!IsValidActiveParty(party))
	{
		for (SessionId sessionId : uniqueMembers)
		{
			_partyBySession.erase(sessionId);
		}
		return PartyResult{ .error = PartyError::InvalidPartyState };
	}

	_parties.emplace(partyId, std::move(party));

	return PartyResult{ .error = PartyError::None, .partyId = partyId };
}

bool PartyService::IsValidActiveParty(const PartyRecord& party) const noexcept
{
	if (party.partyId == 0 ||
		party.lifecycle == PartyLifecycleState::Disbanded ||
		party.members.empty() ||
		party.members.size() > MaxPartyMembers ||
		party.leaderSessionId == 0)
	{
		return false;
	}

	bool foundLeader = false;
	for (const PartyMember& member : party.members)
	{
		if (member.sessionId == 0)
		{
			return false;
		}

		if (member.sessionId == party.leaderSessionId)
		{
			foundLeader = member.role == PartyMemberRole::Leader;
		}
	}

	return foundLeader;
}

bool PartyService::HasPendingJoinRequest(
	const PartyRecord& party,
	SessionId requesterSessionId) const noexcept
{
	return std::any_of(
		party.joinRequests.begin(),
		party.joinRequests.end(),
		[requesterSessionId](const PartyJoinRequest& request)
		{
			return request.requesterSessionId == requesterSessionId &&
				request.state == PartyJoinRequestState::Pending;
		});
}

bool PartyService::IsRejectCooldownActive(
	const PartyRecord& party,
	SessionId requesterSessionId,
	double nowSec) const noexcept
{
	return std::any_of(
		party.joinRequests.begin(),
		party.joinRequests.end(),
		[requesterSessionId, nowSec](const PartyJoinRequest& request)
		{
			return request.requesterSessionId == requesterSessionId &&
				request.state == PartyJoinRequestState::Rejected &&
				request.closeReason == PartyJoinRequestCloseReason::RejectedByLeader &&
				request.closedAtSec > 0.0 &&
				nowSec < request.closedAtSec + PartyRejectCooldownSec;
		});
}

std::vector<SessionId> PartyService::BuildMemberSessionSnapshot(
	const PartyRecord& party) const
{
	std::vector<SessionId> sessionIds;
	sessionIds.reserve(party.members.size());
	for (const PartyMember& member : party.members)
	{
		sessionIds.push_back(member.sessionId);
	}
	std::sort(sessionIds.begin(), sessionIds.end());
	return sessionIds;
}

bool PartyService::HasMemberSession(
	const PartyRecord& party,
	SessionId sessionId) const noexcept
{
	return std::any_of(
		party.members.begin(),
		party.members.end(),
		[sessionId](const PartyMember& member)
		{
			return member.sessionId == sessionId;
		});
}

PartyDeathCountResult PartyService::BuildDeathCountResult(
	const PartyRecord& party,
	PartyError error,
	SessionId consumedBySessionId,
	bool consumed) const noexcept
{
	return PartyDeathCountResult{
		.error = error,
		.partyId = party.partyId,
		.deathCount = party.deathCount,
		.consumedBySessionId = consumedBySessionId,
		.consumed = consumed
	};
}

void PartyService::CloseJoinRequest(
	PartyJoinRequest& request,
	PartyJoinRequestState state,
	PartyJoinRequestCloseReason reason,
	double nowSec) noexcept
{
	if (request.state != PartyJoinRequestState::Pending)
	{
		return;
	}

	request.state = state;
	request.closeReason = reason;
	request.closedAtSec = nowSec;
}

void PartyService::ClosePendingRequestsForParty(
	PartyRecord& party,
	PartyJoinRequestCloseReason reason,
	double nowSec) noexcept
{
	PartyJoinRequestState state = PartyJoinRequestState::Cancelled;
	if (reason == PartyJoinRequestCloseReason::Expired)
	{
		state = PartyJoinRequestState::Expired;
	}

	for (PartyJoinRequest& request : party.joinRequests)
	{
		CloseJoinRequest(request, state, reason, nowSec);
	}
}

void PartyService::ClosePendingRequestsByRequester(
	SessionId requesterSessionId,
	PartyId exceptPartyId,
	PartyJoinRequestCloseReason reason,
	double nowSec) noexcept
{
	for (auto& [partyId, party] : _parties)
	{
		if (partyId == exceptPartyId)
		{
			continue;
		}

		for (PartyJoinRequest& request : party.joinRequests)
		{
			if (request.requesterSessionId == requesterSessionId)
			{
				CloseJoinRequest(
					request,
					PartyJoinRequestState::Cancelled,
					reason,
					nowSec);
			}
		}
	}
}

void PartyService::DisbandParty(PartyRecord& party, double nowSec) noexcept
{
	ClosePendingRequestsForParty(
		party,
		PartyJoinRequestCloseReason::ClosedByPartyDisbanded,
		nowSec);

	for (const PartyMember& member : party.members)
	{
		_partyBySession.erase(member.sessionId);
	}

	party.lifecycle = PartyLifecycleState::Disbanded;
	party.leaderSessionId = 0;
}

void PartyService::ReassignLeaderAfterPresenceChange(PartyRecord& party) noexcept
{
	const auto currentLeaderIt = std::find_if(
		party.members.begin(),
		party.members.end(),
		[&party](const PartyMember& member)
		{
			return member.sessionId == party.leaderSessionId;
		});
	if (currentLeaderIt != party.members.end() &&
		currentLeaderIt->presence == PartyMemberPresence::Online)
	{
		return;
	}

	auto newLeaderIt = party.members.end();
	for (auto it = party.members.begin(); it != party.members.end(); ++it)
	{
		if (it->presence != PartyMemberPresence::Online)
		{
			continue;
		}

		if (newLeaderIt == party.members.end() ||
			it->joinedAtSec < newLeaderIt->joinedAtSec ||
			(it->joinedAtSec == newLeaderIt->joinedAtSec &&
				it->accountId < newLeaderIt->accountId) ||
			(it->joinedAtSec == newLeaderIt->joinedAtSec &&
				it->accountId == newLeaderIt->accountId &&
				it->sessionId < newLeaderIt->sessionId))
		{
			newLeaderIt = it;
		}
	}

	if (newLeaderIt == party.members.end())
	{
		return;
	}

	party.leaderSessionId = newLeaderIt->sessionId;
	for (PartyMember& member : party.members)
	{
		member.role = member.sessionId == party.leaderSessionId
			? PartyMemberRole::Leader
			: PartyMemberRole::Member;
	}
}

PartyId PartyService::AllocatePartyId() noexcept
{
	return _nextPartyId++;
}

PartyRequestId PartyService::AllocateRequestId() noexcept
{
	return _nextRequestId++;
}

void PartyService::AssertOwnerThread() const noexcept
{
	assert(
		_ownerThreadId == std::thread::id{} ||
		_ownerThreadId == std::this_thread::get_id());
}
