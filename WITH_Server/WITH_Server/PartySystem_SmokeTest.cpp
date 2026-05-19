#include "pch.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "DemoPartyFormationPolicy.h"
#include "PartyService.h"

namespace
{
	struct FakeSession
	{
		uint64_t accountId{ 0 };
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
			bool eligible = true,
			bool canTransfer = true)
		{
			_sessions[sessionId] = FakeSession{
				.accountId = 1000 + sessionId,
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
		query.Add(12, plaza);
		query.Add(13, plaza);

		PartyService service{ query };
		const PartyResult create = service.CreateParty(10, 1.0);
		assert(create.Succeeded());
		assert(service.RequestJoin(11, create.partyId, 1.1).Succeeded());
		const PartyResult request12 = service.RequestJoin(12, create.partyId, 1.2);
		assert(request12.Succeeded());
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
}
