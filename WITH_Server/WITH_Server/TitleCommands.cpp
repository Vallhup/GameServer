#include "pch.h"
#include "TitleCommands.h"

#include <limits>

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

GetAccountTitlesCommand::GetAccountTitlesCommand(
	uint64_t accountId,
	uint32_t sessionId,
	uint32_t clientRequestId)
	: _accountId(accountId)
	, _sessionId(sessionId)
	, _clientRequestId(clientRequestId)
{
}

void GetAccountTitlesCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_GetAccountTitles(?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindInt64(1, static_cast<int64_t>(_accountId)) ||
		!stmt.Execute())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	GetAccountTitlesPayload payload{};
	payload.accountId = _accountId;
	payload.sessionId = _sessionId;
	payload.clientRequestId = _clientRequestId;

	while (stmt.Fetch())
	{
		int32_t titleIdRaw{ 0 };
		if (!stmt.GetInt32(1, titleIdRaw) ||
			titleIdRaw <= 0 ||
			titleIdRaw > static_cast<int32_t>(
				std::numeric_limits<TitleId>::max()))
		{
			ctx.CompleteError(
				static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
				L"sp_GetAccountTitles: invalid title_id.");
			return;
		}

		payload.ownedTitleIds.push_back(
			static_cast<TitleId>(titleIdRaw));
	}

	if (!stmt.LastErrorInfo().Empty())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	if (!stmt.MoreResults())
	{
		if (!stmt.LastErrorInfo().Empty())
		{
			CompleteQueryError(ctx, stmt);
		}
		else
		{
			ctx.CompleteError(
				static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
				L"sp_GetAccountTitles: equipped title result set missing.");
		}
		return;
	}

	if (!stmt.Fetch())
	{
		if (!stmt.LastErrorInfo().Empty())
		{
			CompleteQueryError(ctx, stmt);
		}
		else
		{
			ctx.CompleteError(
				static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
				L"sp_GetAccountTitles: account result row missing.");
		}
		return;
	}

	int32_t equippedTitleIdRaw{ 0 };
	if (!stmt.GetInt32(1, equippedTitleIdRaw) ||
		equippedTitleIdRaw < 0 ||
		equippedTitleIdRaw > static_cast<int32_t>(
			std::numeric_limits<TitleId>::max()))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_GetAccountTitles: invalid equipped_title_id.");
		return;
	}

	payload.equippedTitleId =
		static_cast<TitleId>(equippedTitleIdRaw);
	ctx.CompleteOk(std::move(payload));
}

SetEquippedTitleCommand::SetEquippedTitleCommand(
	uint64_t accountId,
	TitleId titleId,
	uint32_t sessionId,
	uint32_t clientRequestId)
	: _accountId(accountId)
	, _titleId(titleId)
	, _sessionId(sessionId)
	, _clientRequestId(clientRequestId)
{
}

void SetEquippedTitleCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_SetEquippedTitle(?, ?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindInt64(1, static_cast<int64_t>(_accountId)) ||
		!stmt.BindInt16(2, static_cast<int16_t>(_titleId)) ||
		!stmt.Execute() ||
		!stmt.Fetch())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	int32_t resultCodeRaw{ 0 };
	int32_t equippedTitleIdRaw{ 0 };
	if (!stmt.GetInt32(1, resultCodeRaw) ||
		!stmt.GetInt32(2, equippedTitleIdRaw) ||
		resultCodeRaw < 0 ||
		equippedTitleIdRaw < 0 ||
		equippedTitleIdRaw > static_cast<int32_t>(
			std::numeric_limits<TitleId>::max()))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_SetEquippedTitle: invalid result set.");
		return;
	}

	SetEquippedTitlePayload payload{};
	payload.accountId = _accountId;
	payload.sessionId = _sessionId;
	payload.clientRequestId = _clientRequestId;
	payload.resultCode = static_cast<uint32_t>(resultCodeRaw);
	payload.equippedTitleId =
		static_cast<TitleId>(equippedTitleIdRaw);
	ctx.CompleteOk(std::move(payload));
}
