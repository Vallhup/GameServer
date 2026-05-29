#pragma once

#include <cstdint>

#include "DBData.h"

constexpr DBCommandTypeId kIncrementMonsterKillCountCommandTypeId    = 3;
constexpr DBCommandTypeId kIncrementDeathByMonsterCountCommandTypeId = 4;
constexpr DBPayloadTypeId kIncrementMonsterKillCountPayloadTypeId    = 3;
constexpr DBPayloadTypeId kIncrementDeathByMonsterCountPayloadTypeId = 4;

// ---------------------------------------------------------------------------
// IncrementMonsterKillCountPayload
//
// sp_IncrementMonsterKillCount 완료 시 서버에 반환되는 데이터.
// 완료 핸들러에서 TitleDefRegistry 조건 검사에 사용한다.
// ---------------------------------------------------------------------------
struct IncrementMonsterKillCountPayload
{
	static constexpr DBPayloadTypeId PayloadTypeId =
		kIncrementMonsterKillCountPayloadTypeId;

	uint64_t accountId{ 0 };
	uint8_t  characterTypeId{ 0 };   // static_cast<uint8_t>(CharacterId)
	uint32_t sessionId{ 0 };
	int32_t  newKillCount{ 0 };
};

// ---------------------------------------------------------------------------
// IncrementMonsterKillCountCommand
//
// AccountMonsterStatistics 테이블의 킬 카운트를 1 증가시키고
// 변경 후 새 카운트를 반환한다.
// ---------------------------------------------------------------------------
class IncrementMonsterKillCountCommand final : public IDBCommand
{
public:
	IncrementMonsterKillCountCommand(
		uint64_t accountId,
		uint8_t  characterTypeId,
		uint32_t sessionId);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kIncrementMonsterKillCountCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "IncrementMonsterKillCount";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	uint64_t _accountId;
	uint8_t  _characterTypeId;
	uint32_t _sessionId;
};

// ---------------------------------------------------------------------------
// IncrementDeathByMonsterCountPayload
// ---------------------------------------------------------------------------
struct IncrementDeathByMonsterCountPayload
{
	static constexpr DBPayloadTypeId PayloadTypeId =
		kIncrementDeathByMonsterCountPayloadTypeId;

	uint64_t accountId{ 0 };
	uint8_t  characterTypeId{ 0 };
	uint32_t sessionId{ 0 };
	int32_t  newDeathCount{ 0 };
};

// ---------------------------------------------------------------------------
// IncrementDeathByMonsterCountCommand
//
// AccountDeathByMonsterStatistics 테이블의 사망 카운트를 1 증가시키고
// 변경 후 새 카운트를 반환한다.
// ---------------------------------------------------------------------------
class IncrementDeathByMonsterCountCommand final : public IDBCommand
{
public:
	IncrementDeathByMonsterCountCommand(
		uint64_t accountId,
		uint8_t  characterTypeId,
		uint32_t sessionId);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kIncrementDeathByMonsterCountCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "IncrementDeathByMonsterCount";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	uint64_t _accountId;
	uint8_t  _characterTypeId;
	uint32_t _sessionId;
};
