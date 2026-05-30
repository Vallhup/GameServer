#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "PartyTypes.h"

// ---------------------------------------------------------------------------
// Party DB 영속화 공용 타입
//
// DB worker thread와 main tick(gateway) 사이를 잇는 데이터 구조를 모은다.
// 파티 상태의 유일한 writer는 main tick이므로, worker는 PartyService를 직접
// 만지지 않고 PartyDbResultQueue(=service mailbox)로만 결과를 돌려준다.
// ---------------------------------------------------------------------------

// Upsert SP에 넘길, DB 표현으로 이미 변환된 파티 한 건.
// joinedAtUtcIso는 항상 채워지고, lastSeenAtUtcIso가 비면 DB에 NULL로 저장된다.
struct PartyPersistMemberRow
{
	uint64_t accountId{ 0 };
	uint8_t role{ 0 };
	uint8_t presence{ 0 };
	std::wstring joinedAtUtcIso;
	std::wstring lastSeenAtUtcIso;
};

struct PartyPersistData
{
	PartyId partyId{ 0 };
	uint64_t leaderAccountId{ 0 };
	uint8_t lifecycle{ 0 };
	uint64_t version{ 0 };
	std::vector<PartyPersistMemberRow> members;
};

enum class PartyDbResultKind : uint8_t
{
	PersistAck,
	SoftDeleteAck,
	LoadActiveParties,
	MaxPartyId
};

// worker → main 으로 전달되는 DB 작업 결과.
struct PartyDbResult
{
	PartyDbResultKind kind{ PartyDbResultKind::PersistAck };

	// PersistAck / SoftDeleteAck
	PartyId partyId{ 0 };
	uint64_t version{ 0 };
	bool success{ false };
	bool stale{ false };

	// LoadActiveParties
	std::vector<RestoredParty> restoredParties;

	// LoadActiveParties / MaxPartyId
	uint64_t maxPartyId{ 0 };
};

// 다중 생산자(worker) → 단일 소비자(main tick) thread-safe mailbox.
// PartyCommandQueue와 동일한 사용 패턴을 따른다.
class PartyDbResultQueue final
{
public:
	void Submit(PartyDbResult result)
	{
		std::lock_guard lock{ _mutex };
		_results.push_back(std::move(result));
	}

	void DrainInto(std::vector<PartyDbResult>& out)
	{
		std::lock_guard lock{ _mutex };
		out.reserve(out.size() + _results.size());
		while (!_results.empty())
		{
			out.push_back(std::move(_results.front()));
			_results.pop_front();
		}
	}

	void Clear()
	{
		std::lock_guard lock{ _mutex };
		_results.clear();
	}

	bool Empty() const
	{
		std::lock_guard lock{ _mutex };
		return _results.empty();
	}

private:
	mutable std::mutex _mutex;
	std::deque<PartyDbResult> _results;
};
