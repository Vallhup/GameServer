#pragma once

#include <cstdint>
#include <vector>

#include "NetId.h"
#include "Session.h"
#include "WorldId.h"
#include "WorldIds.h"
#include "WorldTargetSpec.h"
#include "WorldTransferRequest.h"

inline constexpr uint32_t MaxPartyMembers = 3;

using PartyRequestId = uint64_t;

enum class PartyFormationSource : uint8_t
{
	ExplicitCreate,
	JoinRequestAccepted,
	DemoAutoWorldTransition,
	RestoredFromDB
};

enum class PartyLifecycleState : uint8_t
{
	Forming,
	WorldEntryPending,
	InWorld,
	Disbanded
};

enum class PartyMemberRole : uint8_t
{
	Leader,
	Member
};

enum class PartyJoinRequestState : uint8_t
{
	Pending,
	Accepted,
	Rejected,
	Cancelled,
	Expired
};

enum class PartyWorldEntryState : uint8_t
{
	None,
	Requested,
	TransferEnqueued,
	Completed,
	Failed
};

enum class PartyError : uint8_t
{
	None,
	InvalidSession,
	AlreadyInParty,
	PartyNotFound,
	NotPartyLeader,
	PartyFull,
	AlreadyRequested,
	RequestNotFound,
	RequestNotPending,
	InvalidPartyState,
	WorldEntryAlreadyPending,
	RequesterUnavailable,
	MemberUnavailable,
	LeaderUnavailable,
	SourceWorldMismatch,
	TransferRejected,
	InvalidTarget,
	DuplicateMember
};

struct PartyMember
{
	SessionId sessionId{ 0 };
	uint64_t accountId{ 0 };
	NetId netId{ NetId::Invalid() };
	PartyMemberRole role{ PartyMemberRole::Member };
	double joinedAtSec{ 0.0 };
};

struct PartyJoinRequest
{
	PartyRequestId requestId{ 0 };
	PartyId partyId{ 0 };
	SessionId requesterSessionId{ 0 };
	uint64_t requesterAccountId{ 0 };
	PartyJoinRequestState state{ PartyJoinRequestState::Pending };
	double createdAtSec{ 0.0 };
	double expiresAtSec{ 0.0 };
};

struct PartyWorldEntry
{
	PartyWorldEntryState state{ PartyWorldEntryState::None };
	TransferId transferId{ 0 };
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldTargetSpec target;
	std::vector<SessionId> sessionSnapshot;
	double requestedAtSec{ 0.0 };
	double completedAtSec{ 0.0 };
};

struct PartyRecord
{
	PartyId partyId{ 0 };
	PartyFormationSource formationSource{ PartyFormationSource::ExplicitCreate };
	PartyLifecycleState lifecycle{ PartyLifecycleState::Forming };
	SessionId leaderSessionId{ 0 };
	std::vector<PartyMember> members;
	std::vector<PartyJoinRequest> joinRequests;
	PartyWorldEntry worldEntry;
	double createdAtSec{ 0.0 };
};

struct PartyResult
{
	PartyError error{ PartyError::None };
	PartyId partyId{ 0 };
	PartyRequestId requestId{ 0 };

	bool Succeeded() const noexcept
	{
		return error == PartyError::None;
	}
};

struct PartyWorldEntryResult
{
	PartyError error{ PartyError::None };
	PartyId partyId{ 0 };
	WorldTransferRequest request;

	bool Succeeded() const noexcept
	{
		return error == PartyError::None;
	}
};
