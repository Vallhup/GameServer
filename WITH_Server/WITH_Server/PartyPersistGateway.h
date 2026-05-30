#pragma once

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "PartyPersistTypes.h"

class ODBCDatabaseBackend;
class PartyService;

// ---------------------------------------------------------------------------
// PartyPersistGateway
//
// 파티 도메인(runtime)과 DB 영속화 사이의 단일 통로.
// - main tick에서만 호출된다(PartyService와 같은 owner thread).
// - 변경된 파티 snapshot을 DB 표현으로 변환해 비동기 DB 명령으로 제출한다.
// - DB worker가 PartyDbResultQueue로 돌려준 결과를 main tick에서 흡수한다.
// - 같은 partyId의 더 오래된 제출이 최신 상태를 덮어쓰지 않도록 version으로
//   coalesce하고, 실패한 저장은 backoff 후 재제출한다.
//
// 정책: memory-first. DB 실패는 runtime 상태를 rollback하지 않는다.
// 서버 종료 시 남은 retry job은 유실될 수 있다(허용).
// ---------------------------------------------------------------------------
class PartyPersistGateway final
{
public:
	PartyPersistGateway(
		ODBCDatabaseBackend& database,
		PartyDbResultQueue& resultQueue,
		PartyService& partyService);

	PartyPersistGateway(const PartyPersistGateway&) = delete;
	PartyPersistGateway& operator=(const PartyPersistGateway&) = delete;

	// 서버 시작 시 1회: active 파티 멤버십과 최대 party_id를 비동기로 요청한다.
	void SubmitStartupLoad();

	// 파티 상태 변경을 DB에 반영(upsert)한다. snapshot.version 기준으로 coalesce.
	void PersistParty(const PartySnapshot& snapshot, double nowSec);

	// 파티 해산(soft delete)을 DB에 반영한다.
	void SoftDeleteParty(PartyId partyId, uint64_t version, double nowSec);

	// 현재 메모리 상태를 읽어 upsert/soft-delete 중 알맞은 쪽으로 저장한다.
	// 파티 mutation을 수행한 모든 경로(pump, 데모 전이, transfer 이벤트)가
	// 변경 직후 이 한 메서드만 호출하면 영속화 결정이 한 곳에 모인다.
	void PersistCurrentState(PartyId partyId, double nowSec);

	// 매 main tick: DB 결과를 흡수하고 backoff가 만료된 retry job을 재제출한다.
	void Process(double nowSec);

private:
	struct PendingJob
	{
		bool softDelete{ false };
		uint64_t version{ 0 };
		PartyPersistData data;
		uint32_t retryCount{ 0 };
		double nextRetryAtSec{ 0.0 };
		bool awaitingResult{ false };
	};

	PartyPersistData BuildPersistData(
		const PartySnapshot& snapshot,
		double nowSec) const;

	void SubmitJob(const PendingJob& job);
	void ApplyResult(const PartyDbResult& result, double nowSec);

	static double ComputeBackoffSec(uint32_t retryCount) noexcept;
	static std::wstring FormatUtcIso8601(
		std::chrono::system_clock::time_point timePoint);

private:
	ODBCDatabaseBackend& _database;
	PartyDbResultQueue& _resultQueue;
	PartyService& _partyService;

	std::unordered_map<PartyId, PendingJob> _jobs;
	std::unordered_map<PartyId, uint64_t> _lastSubmittedVersion;
	std::vector<PartyDbResult> _resultScratch;
};
