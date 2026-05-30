#pragma once

#include <vector>

#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "PartyCommandQueue.h"
#include "PartyService.h"

class PartyPersistGateway;

class PartyCommandPump final
{
public:
	PartyCommandPump(
		PartyCommandQueue& queue,
		PartyService& partyService,
		FrameworkRuntime& framework,
		NetworkRuntime& network);

	// DB가 활성화된 경우에만 설정된다. null이면 영속화 없이 메모리 도메인만 동작.
	void SetPersistGateway(PartyPersistGateway* persistGateway) noexcept;

	void Pump(double nowSec);

private:
	void ApplyCommand(const PartyCommand& command, double nowSec);
	void SendUiBootstrap(const PartyCommand& command);
	void SendListSnapshot(const PartyCommand& command);
	void CreateParty(const PartyCommand& command, double nowSec);
	void RequestJoin(const PartyCommand& command, double nowSec);
	void AcceptJoinRequest(const PartyCommand& command, double nowSec);
	void RejectJoinRequest(const PartyCommand& command, double nowSec);
	void MarkMemberOffline(const PartyCommand& command, double nowSec);
	void BeginWorldEntry(const PartyCommand& command, double nowSec);
	void RebindRestoredMember(const PartyCommand& command, double nowSec);
	// 파티 변경을 DB에 반영한다. gateway가 없으면(=DB 비활성) 아무 일도 하지 않는다.
	void PersistPartyState(PartyId partyId, double nowSec);
	void StagePartySnapshotToMembers(const PartySnapshot& snapshot);
	void RegisterPartyListSubscriber(SessionId sessionId);
	void RemovePartyListSubscriber(SessionId sessionId);
	void BroadcastPartyListSnapshot();

private:
	PartyCommandQueue& _queue;
	PartyService& _partyService;
	FrameworkRuntime& _framework;
	NetworkRuntime& _network;
	PartyPersistGateway* _persistGateway{ nullptr };
	std::vector<PartyCommand> _scratch;
	std::vector<PartyListEntry> _listScratch;
	std::vector<SessionId> _partyListSubscribers;
};
