#include "pch.h"
#include "RegisterAccount.h"

namespace
{
	std::wstring WidenAscii(std::string_view value)
	{
		std::wstring wide;
		wide.reserve(value.size());
		for (const unsigned char ch : value)
			wide.push_back(static_cast<wchar_t>(ch));
		return wide;
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

RegisterAccountCommand::RegisterAccountCommand(
	std::wstring loginId,
	std::wstring loginIdNormalized,
	std::string  password)
	: _loginId(std::move(loginId))
	, _loginIdNormalized(std::move(loginIdNormalized))
	, _password(std::move(password))
{
}

void RegisterAccountCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_AccountRegister(?, ?, ?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindString(1, _loginId,           64)  ||
		!stmt.BindString(2, _loginIdNormalized,  64)  ||
		!stmt.BindString(3, WidenAscii(_password), 255) ||
		!stmt.Execute() ||
		!stmt.Fetch())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	int64_t accountIdRaw{ 0 };
	int32_t resultCode{ -1 };
	if (!stmt.GetInt64(1, accountIdRaw) ||
		!stmt.GetInt32(2, resultCode))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_AccountRegister: failed to read result set.");
		return;
	}

	RegisterAccountPayload payload{};
	switch (resultCode) {
	case 0: // 등록 성공
	{
		payload.accountId = static_cast<uint64_t>(accountIdRaw);
		break;
	}
	case 3: // UNIQUE 제약 위반 — 동시 요청으로 이미 등록된 ID
	{
		payload.failReason =
			static_cast<uint32_t>(RegisterAccountFailReason::DuplicateId);
		break;
	}
	default:
	{
		payload.failReason =
			static_cast<uint32_t>(RegisterAccountFailReason::DatabaseError);
		break;
	}
	}

	ctx.CompleteOk(std::move(payload));
}
