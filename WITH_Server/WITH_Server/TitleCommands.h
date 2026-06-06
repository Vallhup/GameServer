#pragma once

#include <cstdint>
#include <vector>

#include "DBData.h"
#include "TitleDef.h"

constexpr DBCommandTypeId kUnlockTitleCommandTypeId = 5;
constexpr DBPayloadTypeId kUnlockTitlePayloadTypeId = 5;
constexpr DBCommandTypeId kGetAccountTitlesCommandTypeId = 10;
constexpr DBCommandTypeId kSetEquippedTitleCommandTypeId = 11;
constexpr DBPayloadTypeId kGetAccountTitlesPayloadTypeId = 6;
constexpr DBPayloadTypeId kSetEquippedTitlePayloadTypeId = 7;

// ---------------------------------------------------------------------------
// UnlockTitlePayload
//
// sp_UnlockTitle 완료 시 서버에 반환되는 데이터.
// newlyUnlocked == true 이면 이번 요청에서 최초로 잠금 해제된 것이다.
// ---------------------------------------------------------------------------
struct UnlockTitlePayload
{
	static constexpr DBPayloadTypeId PayloadTypeId = kUnlockTitlePayloadTypeId;

	TitleId  titleId{ InvalidTitleId };
	uint32_t sessionId{ 0 };
	bool     newlyUnlocked{ false };
};

// ---------------------------------------------------------------------------
// UnlockTitleCommand
//
// AccountTitle 테이블에 (account_id, title_id) 를 INSERT (이미 존재하면 무시).
// SP 결과로 최초 잠금 해제 여부(newly_unlocked)를 반환한다.
// ---------------------------------------------------------------------------
class UnlockTitleCommand final : public IDBCommand
{
public:
	UnlockTitleCommand(uint64_t accountId, TitleId titleId, uint32_t sessionId);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kUnlockTitleCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "UnlockTitle";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	uint64_t _accountId;
	TitleId  _titleId;
	uint32_t _sessionId;
};

struct GetAccountTitlesPayload
{
	static constexpr DBPayloadTypeId PayloadTypeId =
		kGetAccountTitlesPayloadTypeId;

	uint64_t accountId{ 0 };
	uint32_t sessionId{ 0 };
	uint32_t clientRequestId{ 0 };
	std::vector<TitleId> ownedTitleIds;
	TitleId equippedTitleId{ InvalidTitleId };
};

class GetAccountTitlesCommand final : public IDBCommand
{
public:
	GetAccountTitlesCommand(
		uint64_t accountId,
		uint32_t sessionId,
		uint32_t clientRequestId);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kGetAccountTitlesCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "GetAccountTitles";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	uint64_t _accountId;
	uint32_t _sessionId;
	uint32_t _clientRequestId;
};

struct SetEquippedTitlePayload
{
	static constexpr DBPayloadTypeId PayloadTypeId =
		kSetEquippedTitlePayloadTypeId;

	uint64_t accountId{ 0 };
	uint32_t sessionId{ 0 };
	uint32_t clientRequestId{ 0 };
	uint32_t resultCode{ 0 };
	TitleId equippedTitleId{ InvalidTitleId };
};

class SetEquippedTitleCommand final : public IDBCommand
{
public:
	SetEquippedTitleCommand(
		uint64_t accountId,
		TitleId titleId,
		uint32_t sessionId,
		uint32_t clientRequestId);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kSetEquippedTitleCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "SetEquippedTitle";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	uint64_t _accountId;
	TitleId _titleId;
	uint32_t _sessionId;
	uint32_t _clientRequestId;
};
