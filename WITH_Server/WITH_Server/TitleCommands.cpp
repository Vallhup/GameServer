#include "pch.h"
#include "TitleCommands.h"

namespace
{
	void CompleteQueryError(DBCommandContext& ctx, const DBStatement& stmt) noexcept
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			stmt.LastError());
	}
}

UnlockTitleCommand::UnlockTitleCommand(
	uint64_t accountId,
	TitleId  titleId,
	uint32_t sessionId)
	: _accountId(accountId)
	, _titleId(titleId)
	, _sessionId(sessionId)
{
}

void UnlockTitleCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_UnlockTitle(?, ?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindInt64(1, static_cast<int64_t>(_accountId))    ||
		!stmt.BindInt16(2, static_cast<int16_t>(_titleId))      ||
		!stmt.Execute()                                         ||
		!stmt.Fetch())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	int32_t newlyUnlockedRaw{ 0 };
	if (!stmt.GetInt32(1, newlyUnlockedRaw))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_UnlockTitle: failed to read newly_unlocked.");
		return;
	}

	UnlockTitlePayload payload{};
	payload.titleId        = _titleId;
	payload.sessionId      = ctx.Meta().sessionId;
	payload.newlyUnlocked  = (newlyUnlockedRaw != 0);

	ctx.CompleteOk(std::move(payload));
}
