#include "pch.h"
#include "LoginAuth.h"

#include <limits>

namespace
{
	constexpr uint32_t kAccountStatusActive = 1;

	std::wstring WidenAscii(std::string_view value)
	{
		std::wstring wide;
		wide.reserve(value.size());
		for (const unsigned char ch : value)
			wide.push_back(static_cast<wchar_t>(ch));
		return wide;
	}

	void CompleteAuthRejected(DBCommandContext& ctx) noexcept
	{
		LoginAuthPayload payload{};
		payload.failReason =
			static_cast<uint32_t>(LoginAuthFailReason::AuthRejected);
		ctx.CompleteOk(std::move(payload));
	}

	void CompleteQueryError(
		DBCommandContext& ctx,
		const DBStatement& stmt) noexcept
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			stmt.LastError());
	}
}

LoginAuthCommand::LoginAuthCommand(
	std::wstring loginIdNormalized,
	std::string password)
	: _loginIdNormalized(std::move(loginIdNormalized))
	, _password(std::move(password))
{
}

void LoginAuthCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"SELECT AccountId, PasswordHash, Status FROM dbo.Account WHERE LoginIdNormalized = ?;" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindString(1, _loginIdNormalized, 64) ||
		!stmt.Execute())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	if (!stmt.Fetch())
	{
		if (!stmt.LastErrorInfo().Empty())
		{
			CompleteQueryError(ctx, stmt);
			return;
		}

		CompleteAuthRejected(ctx);
		return;
	}

	int64_t accountIdRaw{ 0 };
	std::wstring passwordHash;
	int32_t status{ 0 };
	if (!stmt.GetInt64(1, accountIdRaw) ||
		!stmt.GetString(2, passwordHash) ||
		!stmt.GetInt32(3, status))
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	if (accountIdRaw <= 0 ||
		static_cast<uint64_t>(accountIdRaw) >
			std::numeric_limits<uint64_t>::max())
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"Invalid AccountId value.");
		return;
	}

	if (status != static_cast<int32_t>(kAccountStatusActive))
	{
		CompleteAuthRejected(ctx);
		return;
	}

	if (passwordHash != WidenAscii(_password))
	{
		CompleteAuthRejected(ctx);
		return;
	}

	const uint64_t accountId = static_cast<uint64_t>(accountIdRaw);

	DBStatement updateStmt;
	if (ctx.Prepare(
		DBQueryText{ L"UPDATE dbo.Account SET LastLoginAtUtc = SYSUTCDATETIME() WHERE AccountId = ?;" },
		updateStmt))
	{
		(void)updateStmt.BindInt64(1, static_cast<int64_t>(accountId));
		(void)updateStmt.Execute();
	}

	LoginAuthPayload payload{};
	payload.accountId = accountId;
	payload.failReason =
		static_cast<uint32_t>(LoginAuthFailReason::None);
	ctx.CompleteOk(std::move(payload));
}
