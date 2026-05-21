#pragma once

#include <span>
#include <thread>
#include <unordered_map>

#include "PartyTypes.h"

class IPartySessionQuery
{
public:
	virtual ~IPartySessionQuery() = default;

	virtual bool IsPartyEligible(SessionId sessionId) const = 0;
	virtual uint64_t FindAccountId(SessionId sessionId) const = 0;
	virtual NetId FindControlledNetId(SessionId sessionId) const = 0;
	virtual WorldId FindCurrentWorldId(SessionId sessionId) const = 0;
	virtual void CollectSessionsInWorld(
		WorldId worldId,
		std::vector<SessionId>& outSessionIds) const = 0;
	virtual bool CanBeginWorldTransfer(SessionId sessionId) const = 0;
};

class PartyService final
{
public:
	explicit PartyService(IPartySessionQuery& sessionQuery);

	PartyService(const PartyService&) = delete;
	PartyService& operator=(const PartyService&) = delete;

	void Clear() noexcept;
	void BindOwnerThreadForDebug() noexcept;

	PartyResult CreateParty(SessionId leaderSessionId, double nowSec);
	PartyResult CreatePartyFromTrustedMembers(
		SessionId leaderSessionId,
		std::span<const SessionId> memberSessionIds,
		PartyFormationSource source,
		double nowSec);

	PartyResult RequestJoin(SessionId requesterSessionId, PartyId partyId, double nowSec);
	PartyResult AcceptJoinRequest(SessionId leaderSessionId, PartyRequestId requestId, double nowSec);
	PartyResult RejectJoinRequest(SessionId leaderSessionId, PartyRequestId requestId, double nowSec);
	PartyResult MarkMemberPresence(
		SessionId memberSessionId,
		PartyMemberPresence presence,
		double nowSec);
	void ExpireJoinRequests(double nowSec);

	PartyWorldEntryResult BeginWorldEntry(
		SessionId leaderSessionId,
		const WorldTargetSpec& target,
		double nowSec,
		bool allowFallback = false);

	PartyResult MarkWorldEntryEnqueued(
		PartyId partyId,
		TransferId transferId,
		double nowSec);

	PartyResult CompleteWorldEntry(
		PartyId partyId,
		TransferId transferId,
		WorldId targetWorldId,
		double nowSec);

	PartyResult FailWorldEntry(
		PartyId partyId,
		TransferId transferId,
		double nowSec);

	const PartyRecord* FindParty(PartyId partyId) const noexcept;
	PartyRecord* FindParty(PartyId partyId) noexcept;
	PartyId FindPartyBySession(SessionId sessionId) const noexcept;
	SessionId FindLeaderSession(PartyId partyId) const noexcept;
	PartySnapshot BuildPartySnapshot(PartyId partyId) const;
	PartySnapshot BuildPartySnapshotForSession(SessionId sessionId) const;
	void CollectPublicPartyList(
		std::vector<PartyListEntry>& outEntries,
		size_t limit = PartyListSnapshotLimit) const;

private:
	PartyResult CreatePartyInternal(
		SessionId leaderSessionId,
		std::span<const SessionId> memberSessionIds,
		PartyFormationSource source,
		double nowSec);

	bool IsValidActiveParty(const PartyRecord& party) const noexcept;
	bool HasPendingJoinRequest(const PartyRecord& party, SessionId requesterSessionId) const noexcept;
	bool IsRejectCooldownActive(
		const PartyRecord& party,
		SessionId requesterSessionId,
		double nowSec) const noexcept;
	std::vector<SessionId> BuildMemberSessionSnapshot(const PartyRecord& party) const;
	void CloseJoinRequest(
		PartyJoinRequest& request,
		PartyJoinRequestState state,
		PartyJoinRequestCloseReason reason,
		double nowSec) noexcept;
	void ClosePendingRequestsForParty(
		PartyRecord& party,
		PartyJoinRequestCloseReason reason,
		double nowSec) noexcept;
	void ClosePendingRequestsByRequester(
		SessionId requesterSessionId,
		PartyId exceptPartyId,
		PartyJoinRequestCloseReason reason,
		double nowSec) noexcept;
	void DisbandParty(PartyRecord& party, double nowSec) noexcept;
	void ReassignLeaderAfterPresenceChange(PartyRecord& party) noexcept;

	PartyId AllocatePartyId() noexcept;
	PartyRequestId AllocateRequestId() noexcept;
	void AssertOwnerThread() const noexcept;

private:
	IPartySessionQuery& _sessionQuery;
	std::unordered_map<PartyId, PartyRecord> _parties;
	std::unordered_map<SessionId, PartyId> _partyBySession;
	std::unordered_map<PartyRequestId, PartyId> _partyByRequest;
	PartyId _nextPartyId{ 1 };
	PartyRequestId _nextRequestId{ 1 };
	std::thread::id _ownerThreadId{};
};
