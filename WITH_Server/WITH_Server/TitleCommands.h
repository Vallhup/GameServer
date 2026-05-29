#pragma once

#include <cstdint>

#include "DBData.h"
#include "TitleDef.h"

constexpr DBCommandTypeId kUnlockTitleCommandTypeId = 5;
constexpr DBPayloadTypeId kUnlockTitlePayloadTypeId = 5;

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
