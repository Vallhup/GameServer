#include "pch.h"
#include "PartyService.h"

#include <algorithm>

PartyService::PartyService(IPartySessionQuery& sessionQuery)
	: _sessionQuery(sessionQuery)
{
}

void PartyService::Clear() noexcept
{
	_parties.clear();
	_partyBySession.clear();
	_partyByRequest.clear();
	_nextPartyId = 1;
	_nextRequestId = 1;
}

PartyResult PartyService::CreateParty(
	SessionId leaderSessionId,
	double nowSec)
{
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
		.state = PartyJoinRequestState::Pending,
		.createdAtSec = nowSec,
		.expiresAtSec = 0.0
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

	requestIt->state = PartyJoinRequestState::Accepted;
	party->members.push_back(PartyMember{
		.sessionId = requesterSessionId,
		.accountId = _sessionQuery.FindAccountId(requesterSessionId),
		.netId = _sessionQuery.FindControlledNetId(requesterSessionId),
		.role = PartyMemberRole::Member,
		.joinedAtSec = nowSec
	});
	_partyBySession[requesterSessionId] = party->partyId;

	return PartyResult{
		.error = PartyError::None,
		.partyId = party->partyId,
		.requestId = requestId
	};
}

PartyResult PartyService::RejectJoinRequest(
	SessionId leaderSessionId,
	PartyRequestId requestId,
	double /*nowSec*/)
{
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

	requestIt->state = PartyJoinRequestState::Rejected;
	return PartyResult{
		.error = PartyError::None,
		.partyId = party->partyId,
		.requestId = requestId
	};
}

PartyWorldEntryResult PartyService::BeginWorldEntry(
	SessionId leaderSessionId,
	const WorldTargetSpec& target,
	double nowSec,
	bool allowFallback)
{
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

SessionId PartyService::FindLeaderSession(PartyId partyId) const noexcept
{
	const PartyRecord* const party = FindParty(partyId);
	return party != nullptr ? party->leaderSessionId : 0;
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
			.role = sessionId == leaderSessionId
				? PartyMemberRole::Leader
				: PartyMemberRole::Member,
			.joinedAtSec = nowSec
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

PartyId PartyService::AllocatePartyId() noexcept
{
	return _nextPartyId++;
}

PartyRequestId PartyService::AllocateRequestId() noexcept
{
	return _nextRequestId++;
}
