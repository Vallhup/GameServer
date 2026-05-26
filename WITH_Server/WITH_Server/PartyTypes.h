#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "EntityId.h"
#include "NetId.h"
#include "Session.h"
#include "WorldId.h"
#include "WorldIds.h"
#include "WorldTargetSpec.h"
#include "WorldTransferRequest.h"

inline constexpr uint32_t MaxPartyMembers = 3;
inline constexpr double PartyJoinRequestTimeoutSec = 60.0;
inline constexpr double PartyRejectCooldownSec = 15.0;
inline constexpr size_t PartyListSnapshotLimit = 10;
inline constexpr uint32_t DeathCountPerPartyMember = 5;

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

enum class PartyMemberPresence : uint8_t
{
	Online,
	Offline
};

enum class PartyJoinRequestState : uint8_t
{
	Pending,
	Accepted,
	Rejected,
	Cancelled,
	Expired
};

enum class PartyJoinRequestCloseReason : uint8_t
{
	None,
	Accepted,
	RejectedByLeader,
	CancelledByRequester,
	Expired,
	ClosedByPartyFull,
	ClosedByPartyEnteredWorld,
	ClosedByRequesterJoinedOtherParty,
	ClosedByPartyDisbanded
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
	DuplicateMember,
	RequestCooldown
};

struct PartyMember
{
	SessionId sessionId{ 0 };
	uint64_t accountId{ 0 };
	NetId netId{ NetId::Invalid() };
	CharacterId characterId{ CharacterId::None };
	PartyMemberRole role{ PartyMemberRole::Member };
	PartyMemberPresence presence{ PartyMemberPresence::Online };
	double joinedAtSec{ 0.0 };
	double lastSeenAtSec{ 0.0 };
};

struct PartyJoinRequest
{
	PartyRequestId requestId{ 0 };
	PartyId partyId{ 0 };
	SessionId requesterSessionId{ 0 };
	uint64_t requesterAccountId{ 0 };
	CharacterId requesterCharacterId{ CharacterId::None };
	PartyJoinRequestState state{ PartyJoinRequestState::Pending };
	PartyJoinRequestCloseReason closeReason{ PartyJoinRequestCloseReason::None };
	double createdAtSec{ 0.0 };
	double expiresAtSec{ 0.0 };
	double closedAtSec{ 0.0 };
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

struct PartyDeathCountState
{
	uint32_t initialCount{ 0 };
	uint32_t remainingCount{ 0 };
	uint64_t revision{ 0 };
	double initializedAtSec{ 0.0 };
	double updatedAtSec{ 0.0 };
	bool initialized{ false };
	bool exhausted{ false };
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
	PartyDeathCountState deathCount;
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

struct PartyMemberSnapshot
{
	SessionId sessionId{ 0 };
	uint64_t accountId{ 0 };
	NetId netId{ NetId::Invalid() };
	CharacterId characterId{ CharacterId::None };
	PartyMemberRole role{ PartyMemberRole::Member };
	PartyMemberPresence presence{ PartyMemberPresence::Online };
	double joinedAtSec{ 0.0 };
	double lastSeenAtSec{ 0.0 };
};

struct PartyJoinRequestSnapshot
{
	PartyRequestId requestId{ 0 };
	PartyId partyId{ 0 };
	SessionId requesterSessionId{ 0 };
	uint64_t requesterAccountId{ 0 };
	CharacterId requesterCharacterId{ CharacterId::None };
	PartyJoinRequestState state{ PartyJoinRequestState::Pending };
	PartyJoinRequestCloseReason closeReason{ PartyJoinRequestCloseReason::None };
	double createdAtSec{ 0.0 };
	double expiresAtSec{ 0.0 };
	double closedAtSec{ 0.0 };
};

struct PartySnapshot
{
	PartyId partyId{ 0 };
	PartyFormationSource formationSource{ PartyFormationSource::ExplicitCreate };
	PartyLifecycleState lifecycle{ PartyLifecycleState::Forming };
	SessionId leaderSessionId{ 0 };
	std::vector<PartyMemberSnapshot> members;
	std::vector<PartyJoinRequestSnapshot> joinRequests;
	PartyWorldEntry worldEntry;
	double createdAtSec{ 0.0 };
	bool joinable{ false };
};

struct PartyDeathCountResult
{
	PartyError error{ PartyError::None };
	PartyId partyId{ 0 };
	PartyDeathCountState deathCount;
	SessionId consumedBySessionId{ 0 };
	bool consumed{ false };

	bool Succeeded() const noexcept
	{
		return error == PartyError::None;
	}
};

struct PartyListEntry
{
	PartyId partyId{ 0 };
	SessionId leaderSessionId{ 0 };
	CharacterId leaderCharacterId{ CharacterId::None };
	uint32_t memberCount{ 0 };
	uint32_t capacity{ MaxPartyMembers };
	PartyLifecycleState lifecycle{ PartyLifecycleState::Forming };
	double createdAtSec{ 0.0 };
	bool joinable{ false };
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
