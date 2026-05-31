#include "pch.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "DemoPartyFormationPolicy.h"
#include "PartyCommandQueue.h"
#include "PartyService.h"

namespace
{
	struct FakeSession
	{
		uint64_t accountId{ 0 };
		CharacterId characterId{ CharacterId::None };
		NetId netId{ NetId::Invalid() };
		WorldId worldId{ WorldId::Invalid() };
		bool eligible{ true };
		bool canTransfer{ true };
	};

	class FakePartySessionQuery final
		: public IPartySessionQuery
		, public IDemoPartyTransitionQuery
	{
	public:
		void Add(
			SessionId sessionId,
			WorldId worldId,
			CharacterId characterId = CharacterId::Knight,
			bool eligible = true,
			bool canTransfer = true)
		{
			_sessions[sessionId] = FakeSession{
				.accountId = 1000 + sessionId,
				.characterId = characterId,
				.netId = NetId::Create(sessionId, 1),
				.worldId = worldId,
				.eligible = eligible,
				.canTransfer = canTransfer
			};
		}

		void SetWorld(SessionId sessionId, WorldId worldId)
		{
			_sessions[sessionId].worldId = worldId;
		}

		void SetPending(SessionId sessionId, bool pending)
		{
			if (pending)
				_pending.insert(sessionId);
			else
				_pending.erase(sessionId);
		}

		bool IsPartyEligible(SessionId sessionId) const override
		{
			const auto it = _sessions.find(sessionId);
			return it != _sessions.end() && it->second.eligible;
		}

		uint64_t FindAccountId(SessionId sessionId) const override
		{
			const auto it = _sessions.find(sessionId);
			return it != _sessions.end() ? it->second.accountId : 0;
		}

		CharacterId FindSelectedCharacterId(SessionId sessionId) const override
		{
			const auto it = _sessions.find(sessionId);
			return it != _sessions.end() ? it->second.characterId : CharacterId::None;
		}

		NetId FindControlledNetId(SessionId sessionId) const override
		{
			const auto it = _sessions.find(sessionId);
			return it != _sessions.end() ? it->second.netId : NetId::Invalid();
		}

		WorldId FindCurrentWorldId(SessionId sessionId) const override
		{
			const auto it = _sessions.find(sessionId);
			return it != _sessions.end() ? it->second.worldId : WorldId::Invalid();
		}

		void CollectSessionsInWorld(
			WorldId worldId,
			std::vector<SessionId>& outSessionIds) const override
		{
			outSessionIds.clear();
			for (const auto& [sessionId, session] : _sessions)
			{
				if (session.worldId == worldId)
					outSessionIds.push_back(sessionId);
			}
			std::sort(outSessionIds.begin(), outSessionIds.end());
		}

		bool CanBeginWorldTransfer(SessionId sessionId) const override
		{
			const auto it = _sessions.find(sessionId);
			return it != _sessions.end() &&
				it->second.eligible &&
				it->second.canTransfer &&
				_pending.find(sessionId) == _pending.end() &&
				it->second.worldId.IsValid();
		}

		bool IsClientTransitionPending(SessionId sessionId) const override
		{
			return _pending.find(sessionId) != _pending.end();
		}

	private:
		std::unordered_map<SessionId, FakeSession> _sessions;
		std::unordered_set<SessionId> _pending;
	};

	WorldTargetSpec MakeTarget(WorldDefId defId, uint64_t instanceKey)
	{
		WorldTargetSpec target{};
		target.targetWorldDefId = defId;
		target.instanceKey = instanceKey;
		return target;
	}

	void Test_Party_01_DemoAutoParty_CreatesMaxThree()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(1, plaza);
		query.Add(2, plaza);
		query.Add(3, plaza);
		query.Add(4, plaza);

		PartyService service{ query };
		DemoPartyFormationPolicy demoPolicy{ service, query, query };

		const PartyResult result =
			demoPolicy.EnsurePartyForWorldTransition(2, plaza, 1.0);
		assert(result.Succeeded());

		const PartyRecord* const party = service.FindParty(result.partyId);
		assert(party != nullptr);
		assert(party->formationSource == PartyFormationSource::DemoAutoWorldTransition);
		assert(party->leaderSessionId == 2);
		assert(party->members.size() == MaxPartyMembers);
		assert(service.FindPartyBySession(1) == result.partyId);
		assert(service.FindPartyBySession(2) == result.partyId);
		assert(service.FindPartyBySession(3) == result.partyId);
		assert(service.FindPartyBySession(4) == 0);
	}

	void Test_Party_02_WorldEntry_UsesSnapshotAndInstanceKey()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(1, plaza);
		query.Add(2, plaza);
		query.Add(3, plaza);

		PartyService service{ query };
		DemoPartyFormationPolicy demoPolicy{ service, query, query };

		const PartyResult partyResult =
			demoPolicy.EnsurePartyForWorldTransition(1, plaza, 1.0);
		assert(partyResult.Succeeded());

		const WorldTargetSpec target =
			MakeTarget(
				WorldDefId::Village,
				static_cast<uint64_t>(partyResult.partyId));
		PartyWorldEntryResult entry =
			service.BeginWorldEntry(1, target, 2.0, true);
		assert(entry.Succeeded());
		assert(entry.partyId == partyResult.partyId);
		assert(entry.request.partyId == partyResult.partyId);
		assert(entry.request.sourceWorldId == plaza);
		assert(entry.request.target.targetWorldDefId.has_value());
		assert(*entry.request.target.targetWorldDefId == WorldDefId::Village);
		assert(entry.request.target.instanceKey == static_cast<uint64_t>(partyResult.partyId));
		assert(entry.request.allowFallback);
		assert(entry.request.sessionIds.size() == 3);
		assert(entry.request.sessionIds[0] == 1);
		assert(entry.request.sessionIds[1] == 2);
		assert(entry.request.sessionIds[2] == 3);

		const PartyRecord* const party = service.FindParty(partyResult.partyId);
		assert(party != nullptr);
		assert(party->lifecycle == PartyLifecycleState::WorldEntryPending);
		assert(party->worldEntry.state == PartyWorldEntryState::Requested);
	}

	void Test_Party_03_WorldEntry_FailAndCompleteTransitions()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		const WorldId village = WorldId::Create(2, 1);
		FakePartySessionQuery query{};
		query.Add(1, plaza);
		query.Add(2, plaza);

		PartyService service{ query };
		DemoPartyFormationPolicy demoPolicy{ service, query, query };

		const PartyResult partyResult =
			demoPolicy.EnsurePartyForWorldTransition(1, plaza, 1.0);
		assert(partyResult.Succeeded());

		PartyWorldEntryResult firstEntry =
			service.BeginWorldEntry(
				1,
				MakeTarget(WorldDefId::Village, static_cast<uint64_t>(partyResult.partyId)),
				2.0,
				true);
		assert(firstEntry.Succeeded());
		assert(service.FailWorldEntry(partyResult.partyId, 0, 3.0).Succeeded());
		assert(service.FindParty(partyResult.partyId)->lifecycle == PartyLifecycleState::Forming);

		PartyWorldEntryResult secondEntry =
			service.BeginWorldEntry(
				1,
				MakeTarget(WorldDefId::Village, static_cast<uint64_t>(partyResult.partyId)),
				4.0,
				true);
		assert(secondEntry.Succeeded());
		assert(service.MarkWorldEntryEnqueued(partyResult.partyId, 77, 4.1).Succeeded());
		assert(service.CompleteWorldEntry(partyResult.partyId, 77, village, 5.0).Succeeded());
		assert(service.FindParty(partyResult.partyId)->lifecycle == PartyLifecycleState::InWorld);
		assert(service.FindParty(partyResult.partyId)->worldEntry.state == PartyWorldEntryState::Completed);
	}

	void Test_Party_04_SourceMismatchRejected()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		const WorldId village = WorldId::Create(2, 1);
		FakePartySessionQuery query{};
		query.Add(1, plaza);
		query.Add(2, plaza);

		PartyService service{ query };
		DemoPartyFormationPolicy demoPolicy{ service, query, query };

		const PartyResult partyResult =
			demoPolicy.EnsurePartyForWorldTransition(1, plaza, 1.0);
		assert(partyResult.Succeeded());

		query.SetWorld(2, village);
		const PartyWorldEntryResult entry =
			service.BeginWorldEntry(
				1,
				MakeTarget(WorldDefId::Village, static_cast<uint64_t>(partyResult.partyId)),
				2.0,
				true);
		assert(!entry.Succeeded());
		assert(entry.error == PartyError::SourceWorldMismatch);
	}

	void Test_Party_05_JoinRequestFlow_RemainsAvailableForFormalParty()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(10, plaza);
		query.Add(11, plaza);
		query.Add(12, plaza, CharacterId::Lancer);
		query.Add(13, plaza);

		PartyService service{ query };
		const PartyResult create = service.CreateParty(10, 1.0);
		assert(create.Succeeded());
		assert(service.RequestJoin(11, create.partyId, 1.1).Succeeded());
		const PartyResult request12 = service.RequestJoin(12, create.partyId, 1.2);
		assert(request12.Succeeded());
		const PartySnapshot pendingSnapshot =
			service.BuildPartySnapshot(create.partyId);
		const auto request12It = std::find_if(
			pendingSnapshot.joinRequests.begin(),
			pendingSnapshot.joinRequests.end(),
			[&request12](const PartyJoinRequestSnapshot& request)
			{
				return request.requestId == request12.requestId;
			});
		assert(request12It != pendingSnapshot.joinRequests.end());
		assert(request12It->requesterCharacterId == CharacterId::Lancer);
		const PartyResult accepted12 =
			service.AcceptJoinRequest(10, request12.requestId, 1.3);
		assert(accepted12.Succeeded());

		const PartyResult request13 = service.RequestJoin(13, create.partyId, 1.4);
		assert(request13.Succeeded());
		const PartyResult accepted13 =
			service.AcceptJoinRequest(10, request13.requestId, 1.5);
		assert(accepted13.Succeeded());

		assert(service.FindParty(create.partyId)->members.size() == MaxPartyMembers);
		query.Add(14, plaza);
		const PartyResult fullRequest = service.RequestJoin(14, create.partyId, 1.7);
		assert(!fullRequest.Succeeded());
		assert(fullRequest.error == PartyError::PartyFull);
	}

	void Test_Party_06_CommandQueue_DrainsMultiProducer()
	{
		PartyCommandQueue queue{};
		constexpr uint32_t producerCount = 4;
		constexpr uint32_t commandsPerProducer = 32;

		std::vector<std::thread> producers;
		producers.reserve(producerCount);
		for (uint32_t producerIndex = 0; producerIndex < producerCount; ++producerIndex)
		{
			producers.emplace_back(
				[&queue, producerIndex]()
				{
					for (uint32_t commandIndex = 0;
						commandIndex < commandsPerProducer;
						++commandIndex)
					{
						queue.Submit(PartyCommand{
							.kind = PartyCommandKind::CreateParty,
							.actorSessionId =
								static_cast<SessionId>(
									1 + producerIndex * commandsPerProducer + commandIndex),
							.submittedAtSec = static_cast<double>(commandIndex),
							.correlationId =
								static_cast<uint64_t>(
									producerIndex * commandsPerProducer + commandIndex)
						});
					}
				});
		}

		for (std::thread& producer : producers)
		{
			producer.join();
		}

		std::vector<PartyCommand> drained;
		queue.DrainInto(drained);
		assert(drained.size() == producerCount * commandsPerProducer);
		assert(queue.Empty());
	}

	void Test_Party_07_OwnerThreadGuard_AllowsBoundOwner()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(20, plaza);

		PartyService service{ query };
		service.BindOwnerThreadForDebug();

		const PartyResult create = service.CreateParty(20, 1.0);
		assert(create.Succeeded());
		assert(service.FindPartyBySession(20) == create.partyId);
	}

	void Test_Party_08_PendingCloseReasons_TimeoutAndCooldown()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(30, plaza);
		query.Add(40, plaza);
		query.Add(50, plaza);
		query.Add(51, plaza);
		query.Add(52, plaza);
		query.Add(53, plaza);

		PartyService service{ query };
		const PartyResult partyA = service.CreateParty(30, 1.0);
		const PartyResult partyB = service.CreateParty(40, 1.1);
		assert(partyA.Succeeded());
		assert(partyB.Succeeded());

		const PartyResult requestA = service.RequestJoin(50, partyA.partyId, 2.0);
		const PartyResult requestB = service.RequestJoin(50, partyB.partyId, 2.1);
		assert(requestA.Succeeded());
		assert(requestB.Succeeded());

		assert(service.AcceptJoinRequest(30, requestA.requestId, 3.0).Succeeded());
		const PartyRecord* const partyBRecord = service.FindParty(partyB.partyId);
		assert(partyBRecord != nullptr);
		assert(partyBRecord->joinRequests[0].state == PartyJoinRequestState::Cancelled);
		assert(
			partyBRecord->joinRequests[0].closeReason ==
			PartyJoinRequestCloseReason::ClosedByRequesterJoinedOtherParty);

		const PartyResult request51 = service.RequestJoin(51, partyA.partyId, 4.0);
		const PartyResult request52 = service.RequestJoin(52, partyA.partyId, 4.1);
		assert(request51.Succeeded());
		assert(request52.Succeeded());
		assert(service.AcceptJoinRequest(30, request51.requestId, 4.2).Succeeded());
		const PartyRecord* const partyARecord = service.FindParty(partyA.partyId);
		assert(partyARecord != nullptr);
		assert(partyARecord->members.size() == MaxPartyMembers);
		const auto closedByFullIt = std::find_if(
			partyARecord->joinRequests.begin(),
			partyARecord->joinRequests.end(),
			[&request52](const PartyJoinRequest& request)
			{
				return request.requestId == request52.requestId;
			});
		assert(closedByFullIt != partyARecord->joinRequests.end());
		assert(closedByFullIt->state == PartyJoinRequestState::Cancelled);
		assert(closedByFullIt->closeReason == PartyJoinRequestCloseReason::ClosedByPartyFull);

		const PartyResult request53 = service.RequestJoin(53, partyB.partyId, 5.0);
		assert(request53.Succeeded());
		assert(service.RejectJoinRequest(40, request53.requestId, 6.0).Succeeded());
		const PartyResult cooldown = service.RequestJoin(53, partyB.partyId, 10.0);
		assert(!cooldown.Succeeded());
		assert(cooldown.error == PartyError::RequestCooldown);

		const PartyResult afterCooldown = service.RequestJoin(53, partyB.partyId, 22.0);
		assert(afterCooldown.Succeeded());
		service.ExpireJoinRequests(83.0);
		const PartyRecord* const timeoutParty = service.FindParty(partyB.partyId);
		assert(timeoutParty != nullptr);
		const auto expiredIt = std::find_if(
			timeoutParty->joinRequests.begin(),
			timeoutParty->joinRequests.end(),
			[&afterCooldown](const PartyJoinRequest& request)
			{
				return request.requestId == afterCooldown.requestId;
			});
		assert(expiredIt != timeoutParty->joinRequests.end());
		assert(expiredIt->state == PartyJoinRequestState::Expired);
		assert(expiredIt->closeReason == PartyJoinRequestCloseReason::Expired);
	}

	void Test_Party_09_PresenceLeaderHandoffAndDisband()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(60, plaza);
		query.Add(61, plaza);
		query.Add(62, plaza);

		PartyService service{ query };
		const PartyResult partyResult = service.CreateParty(60, 1.0);
		assert(partyResult.Succeeded());
		const PartyResult request61 = service.RequestJoin(61, partyResult.partyId, 2.0);
		const PartyResult request62 = service.RequestJoin(62, partyResult.partyId, 3.0);
		assert(request61.Succeeded());
		assert(request62.Succeeded());
		assert(service.AcceptJoinRequest(60, request61.requestId, 4.0).Succeeded());
		assert(service.AcceptJoinRequest(60, request62.requestId, 5.0).Succeeded());

		assert(
			service.MarkMemberPresence(
				60,
				PartyMemberPresence::Offline,
				6.0).Succeeded());
		const PartyRecord* const party = service.FindParty(partyResult.partyId);
		assert(party != nullptr);
		assert(party->leaderSessionId == 61);

		assert(
			service.MarkMemberPresence(
				61,
				PartyMemberPresence::Offline,
				7.0).Succeeded());
		assert(service.FindParty(partyResult.partyId)->leaderSessionId == 62);
		assert(
			service.MarkMemberPresence(
				62,
				PartyMemberPresence::Offline,
				8.0).Succeeded());
		assert(service.FindParty(partyResult.partyId)->lifecycle == PartyLifecycleState::Disbanded);
		assert(service.FindPartyBySession(60) == 0);
		assert(service.FindPartyBySession(61) == 0);
		assert(service.FindPartyBySession(62) == 0);
	}

	void Test_Party_10_SnapshotAndPublicPartyList()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		for (SessionId sessionId = 100; sessionId < 114; ++sessionId)
		{
			query.Add(sessionId, plaza);
		}

		PartyService service{ query };
		std::vector<PartyId> partyIds;
		for (SessionId sessionId = 100; sessionId < 112; ++sessionId)
		{
			const PartyResult result =
				service.CreateParty(sessionId, static_cast<double>(sessionId));
			assert(result.Succeeded());
			partyIds.push_back(result.partyId);
		}

		const PartySnapshot snapshot = service.BuildPartySnapshot(partyIds.front());
		assert(snapshot.partyId == partyIds.front());
		assert(snapshot.members.size() == 1);
		assert(snapshot.members[0].characterId == CharacterId::Knight);
		assert(snapshot.joinable);

		std::vector<PartyListEntry> list;
		service.CollectPublicPartyList(list);
		assert(list.size() == PartyListSnapshotLimit);
		assert(list[0].leaderSessionId == 111);
		assert(list[0].leaderCharacterId == CharacterId::Knight);
		assert(list[1].leaderSessionId == 110);
		assert(list.back().leaderSessionId == 102);
	}

	void Test_Party_11_DeathCountInitializesAndConsumes()
	{
		const WorldId village = WorldId::Create(2, 77);
		FakePartySessionQuery query{};
		query.Add(200, village);
		query.Add(201, village);
		query.Add(202, village);

		PartyService service{ query };
		const SessionId members[] = { 200, 201, 202 };
		const PartyResult create =
			service.CreatePartyFromTrustedMembers(
				200,
				std::span<const SessionId>(members, 3),
				PartyFormationSource::RestoredFromDB,
				1.0);
		assert(create.Succeeded());

		const PartyDeathCountResult init =
			service.InitializeDeathCountForRun(create.partyId, 2.0);
		assert(init.Succeeded());
		assert(init.deathCount.initialized);
		assert(init.deathCount.initialCount == 15);
		assert(init.deathCount.remainingCount == 15);
		assert(init.deathCount.revision == 1);

		const PartyDeathCountResult firstDeath =
			service.ConsumeDeathCount(create.partyId, 201, 3.0);
		assert(firstDeath.Succeeded());
		assert(firstDeath.consumed);
		assert(firstDeath.deathCount.remainingCount == 14);
		assert(firstDeath.deathCount.revision == 2);

		const PartyDeathCountResult invalidDeath =
			service.ConsumeDeathCount(create.partyId, 999, 4.0);
		assert(!invalidDeath.Succeeded());
		assert(invalidDeath.error == PartyError::InvalidSession);
		assert(service.GetDeathCountSnapshot(create.partyId).remainingCount == 14);

		for (uint32_t i = 0; i < 14; ++i)
		{
			const PartyDeathCountResult result =
				service.ConsumeDeathCount(create.partyId, 200, 5.0 + i);
			assert(result.Succeeded());
		}

		const PartyDeathCountState finalState =
			service.GetDeathCountSnapshot(create.partyId);
		assert(finalState.remainingCount == 0);
		assert(finalState.exhausted);
	}

	// 멤버가 파티에 참가한 상태에서 로그아웃(in-memory, 서버 재시작 없음)한 뒤
	// 새 세션으로 재접속하면, 동일 account의 offline 슬롯에 다시 바인딩되어
	// 파티가 유지되어야 한다. (회귀: Offline 전환 시 sessionId가 0으로 리셋되지
	// 않아 RebindMemberByAccount가 슬롯을 찾지 못하던 버그)
	void Test_Party_12_ReconnectRebindsMemberAfterLogout()
	{
		const WorldId plaza = WorldId::Create(1, 1);
		FakePartySessionQuery query{};
		query.Add(60, plaza);
		query.Add(61, plaza);
		query.Add(71, plaza); // 멤버 61의 재접속 세션

		PartyService service{ query };
		const PartyResult partyResult = service.CreateParty(60, 1.0);
		assert(partyResult.Succeeded());
		const PartyResult request61 =
			service.RequestJoin(61, partyResult.partyId, 2.0);
		assert(request61.Succeeded());
		assert(service.AcceptJoinRequest(60, request61.requestId, 3.0).Succeeded());

		const uint64_t account61 = query.FindAccountId(61);

		// 멤버 61 로그아웃: 리더(60)는 온라인이므로 파티는 해산되면 안 된다.
		assert(
			service.MarkMemberPresence(
				61,
				PartyMemberPresence::Offline,
				4.0).Succeeded());
		const PartyRecord* const party = service.FindParty(partyResult.partyId);
		assert(party != nullptr);
		assert(party->lifecycle != PartyLifecycleState::Disbanded);

		// 옛 세션 매핑은 정리되어 더 이상 조회되면 안 된다.
		assert(service.FindPartyBySession(61) == 0);

		// 새 세션(71)으로 재접속 → 동일 account 슬롯에 rebind 성공해야 한다.
		const PartyResult rebind =
			service.RebindMemberByAccount(account61, 71, 5.0);
		assert(rebind.Succeeded());
		assert(rebind.partyId == partyResult.partyId);
		assert(service.FindPartyBySession(71) == partyResult.partyId);

		const PartyRecord* const rejoined =
			service.FindParty(partyResult.partyId);
		assert(rejoined != nullptr);
		const auto memberIt = std::find_if(
			rejoined->members.begin(),
			rejoined->members.end(),
			[account61](const PartyMember& member)
			{
				return member.accountId == account61;
			});
		assert(memberIt != rejoined->members.end());
		assert(memberIt->sessionId == 71);
		assert(memberIt->presence == PartyMemberPresence::Online);
	}
}

void RunPartySystemSmokeTests()
{
	Test_Party_01_DemoAutoParty_CreatesMaxThree();
	std::cout << "[PASS] Test_Party_01_DemoAutoParty_CreatesMaxThree\n";

	Test_Party_02_WorldEntry_UsesSnapshotAndInstanceKey();
	std::cout << "[PASS] Test_Party_02_WorldEntry_UsesSnapshotAndInstanceKey\n";

	Test_Party_03_WorldEntry_FailAndCompleteTransitions();
	std::cout << "[PASS] Test_Party_03_WorldEntry_FailAndCompleteTransitions\n";

	Test_Party_04_SourceMismatchRejected();
	std::cout << "[PASS] Test_Party_04_SourceMismatchRejected\n";

	Test_Party_05_JoinRequestFlow_RemainsAvailableForFormalParty();
	std::cout << "[PASS] Test_Party_05_JoinRequestFlow_RemainsAvailableForFormalParty\n";

	Test_Party_06_CommandQueue_DrainsMultiProducer();
	std::cout << "[PASS] Test_Party_06_CommandQueue_DrainsMultiProducer\n";

	Test_Party_07_OwnerThreadGuard_AllowsBoundOwner();
	std::cout << "[PASS] Test_Party_07_OwnerThreadGuard_AllowsBoundOwner\n";

	Test_Party_08_PendingCloseReasons_TimeoutAndCooldown();
	std::cout << "[PASS] Test_Party_08_PendingCloseReasons_TimeoutAndCooldown\n";

	Test_Party_09_PresenceLeaderHandoffAndDisband();
	std::cout << "[PASS] Test_Party_09_PresenceLeaderHandoffAndDisband\n";

	Test_Party_10_SnapshotAndPublicPartyList();
	std::cout << "[PASS] Test_Party_10_SnapshotAndPublicPartyList\n";

	Test_Party_11_DeathCountInitializesAndConsumes();
	std::cout << "[PASS] Test_Party_11_DeathCountInitializesAndConsumes\n";

	Test_Party_12_ReconnectRebindsMemberAfterLogout();
	std::cout << "[PASS] Test_Party_12_ReconnectRebindsMemberAfterLogout\n";
}
