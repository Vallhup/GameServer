#pragma once

#include <cstdint>

#include "DBData.h"
#include "PartyPersistTypes.h"

// ---------------------------------------------------------------------------
// Party DB 영속화 명령 (IDBCommand)
//
// 기존 DB 명령(LoginAuth, CombatStatistics 등)과 동일하게 DB worker thread에서
// Execute가 호출된다. 다만 파티 결과는 executor completion 경로가 아니라
// PartyDbResultQueue(service mailbox)로 돌려준다. 따라서 ctx.CompleteOk를
// 호출하지 않으며, Execute는 예외를 던지지 않는다(noexcept 계약 준수).
//
// DBCommandTypeId 1~5는 기존 명령이 사용 중이므로 파티는 6번부터 예약한다.
// ---------------------------------------------------------------------------

constexpr DBCommandTypeId kUpsertPartySnapshotCommandTypeId = 6;
constexpr DBCommandTypeId kSoftDeletePartyCommandTypeId     = 7;
constexpr DBCommandTypeId kLoadActivePartiesCommandTypeId   = 8;
constexpr DBCommandTypeId kGetMaxPartyIdCommandTypeId       = 9;

// sp_UpsertPartySnapshot: 파티 헤더 + active 멤버 snapshot을 단일 트랜잭션 저장.
class UpsertPartySnapshotCommand final : public IDBCommand
{
public:
	UpsertPartySnapshotCommand(
		PartyPersistData data,
		PartyDbResultQueue& resultSink);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kUpsertPartySnapshotCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "UpsertPartySnapshot";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	std::wstring BuildMembersJson() const;

private:
	PartyPersistData _data;
	PartyDbResultQueue& _resultSink;
};

// sp_SoftDeleteParty: 파티 해산을 soft delete로 기록하고 active 멤버를 닫는다.
class SoftDeletePartyCommand final : public IDBCommand
{
public:
	SoftDeletePartyCommand(
		PartyId partyId,
		uint64_t version,
		PartyDbResultQueue& resultSink);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kSoftDeletePartyCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "SoftDeleteParty";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	PartyId _partyId;
	uint64_t _version;
	PartyDbResultQueue& _resultSink;
};

// sp_LoadActiveParties: 서버 시작 시 disbanded_at IS NULL 파티 멤버십을 복구한다.
class LoadActivePartiesCommand final : public IDBCommand
{
public:
	explicit LoadActivePartiesCommand(PartyDbResultQueue& resultSink);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kLoadActivePartiesCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "LoadActiveParties";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	PartyDbResultQueue& _resultSink;
};

// sp_GetMaxPartyId: soft-deleted 포함 최대 party_id (다음 PartyId 발급 초기화용).
class GetMaxPartyIdCommand final : public IDBCommand
{
public:
	explicit GetMaxPartyIdCommand(PartyDbResultQueue& resultSink);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kGetMaxPartyIdCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "GetMaxPartyId";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	PartyDbResultQueue& _resultSink;
};
