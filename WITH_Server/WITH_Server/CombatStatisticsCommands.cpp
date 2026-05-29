#include "pch.h"
#include "CombatStatisticsCommands.h"

namespace
{
	void CompleteQueryError(DBCommandContext& ctx, const DBStatement& stmt) noexcept
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			stmt.LastError());
	}
}

// ---------------------------------------------------------------------------
// IncrementMonsterKillCountCommand
// ---------------------------------------------------------------------------

IncrementMonsterKillCountCommand::IncrementMonsterKillCountCommand(
	uint64_t accountId,
	uint8_t  characterTypeId,
	uint32_t sessionId)
	: _accountId(accountId)
	, _characterTypeId(characterTypeId)
	, _sessionId(sessionId)
{
}

void IncrementMonsterKillCountCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_IncrementMonsterKillCount(?, ?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindInt64(1, static_cast<int64_t>(_accountId))          ||
		!stmt.BindInt16(2, static_cast<int16_t>(_characterTypeId))    ||
		!stmt.Execute()                                                ||
		!stmt.Fetch())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	int32_t newKillCount{ 0 };
	if (!stmt.GetInt32(1, newKillCount))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_IncrementMonsterKillCount: failed to read new_kill_count.");
		return;
	}

	IncrementMonsterKillCountPayload payload{};
	payload.accountId       = _accountId;
	payload.characterTypeId = _characterTypeId;
	payload.sessionId       = ctx.Meta().sessionId;
	payload.newKillCount    = newKillCount;

	ctx.CompleteOk(std::move(payload));
}

// ---------------------------------------------------------------------------
// IncrementDeathByMonsterCountCommand
// ---------------------------------------------------------------------------

IncrementDeathByMonsterCountCommand::IncrementDeathByMonsterCountCommand(
	uint64_t accountId,
	uint8_t  characterTypeId,
	uint32_t sessionId)
	: _accountId(accountId)
	, _characterTypeId(characterTypeId)
	, _sessionId(sessionId)
{
}

void IncrementDeathByMonsterCountCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_IncrementDeathByMonsterCount(?, ?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindInt64(1, static_cast<int64_t>(_accountId))          ||
		!stmt.BindInt16(2, static_cast<int16_t>(_characterTypeId))    ||
		!stmt.Execute()                                                ||
		!stmt.Fetch())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	int32_t newDeathCount{ 0 };
	if (!stmt.GetInt32(1, newDeathCount))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_IncrementDeathByMonsterCount: failed to read new_death_count.");
		return;
	}

	IncrementDeathByMonsterCountPayload payload{};
	payload.accountId       = _accountId;
	payload.characterTypeId = _characterTypeId;
	payload.sessionId       = ctx.Meta().sessionId;
	payload.newDeathCount   = newDeathCount;

	ctx.CompleteOk(std::move(payload));
}
