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
	virtual CharacterId FindSelectedCharacterId(SessionId sessionId) const = 0;
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

	// --- DB 복구 경로 ---------------------------------------------------
	// DB snapshot 하나를 메모리 파티로 복원한다. session/netId/위치는 복원하지
	// 않으며 모든 멤버 presence는 Offline으로 시작한다. lifecycle은 정규화된다.
	PartyResult RestorePartyFromSnapshot(
		const RestoredParty& restored,
		double nowSec);

	// 복구가 끝난 뒤 다음 PartyId 발급값을 DB 최대 id 기준으로 끌어올린다.
	void AdvanceNextPartyIdTo(PartyId maxRestoredPartyId) noexcept;

	// 재접속한 account를 복구된 파티의 offline 멤버 슬롯에 다시 바인딩한다.
	// 필요 시 첫 접속 멤버에게 leader를 위임한다.
	PartyResult RebindMemberByAccount(
		uint64_t accountId,
		SessionId sessionId,
		double nowSec);

	PartyWorldEntryResult BeginWorldEntry(
		SessionId leaderSessionId,
		const WorldTargetSpec& target,
		double nowSec,
		bool allowFallback = false);

	// 강제(리더 비의존) 그룹 전송용. PvP 라운드 종료/엔딩 복귀처럼 리더가
	// 사망/접속종료 상태일 수 있는 상황에서, 지정 소스월드에 실재하며 전송
	// 가능한 멤버(사망자 포함, 접속종료 제외)만 모아 전송을 시작한다.
	PartyWorldEntryResult BeginForcedWorldEntry(
		PartyId partyId,
		WorldId sourceWorldId,
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

	PartyDeathCountResult InitializeDeathCountForRun(
		PartyId partyId,
		double nowSec);

	PartyDeathCountResult ConsumeDeathCount(
		PartyId partyId,
		SessionId deadSessionId,
		double nowSec);

	const PartyRecord* FindParty(PartyId partyId) const noexcept;
	PartyRecord* FindParty(PartyId partyId) noexcept;
	PartyId FindPartyBySession(SessionId sessionId) const noexcept;
	PartyId FindPartyByRequest(PartyRequestId requestId) const noexcept;
	const PartyJoinRequest* FindJoinRequest(PartyRequestId requestId) const noexcept;
	SessionId FindLeaderSession(PartyId partyId) const noexcept;
	PartySnapshot BuildPartySnapshot(PartyId partyId) const;
	PartySnapshot BuildPartySnapshotForSession(SessionId sessionId) const;
	PartyDeathCountState GetDeathCountSnapshot(PartyId partyId) const noexcept;
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
	bool HasMemberSession(const PartyRecord& party, SessionId sessionId) const noexcept;
	PartyDeathCountResult BuildDeathCountResult(
		const PartyRecord& party,
		PartyError error = PartyError::None,
		SessionId consumedBySessionId = 0,
		bool consumed = false) const noexcept;
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
