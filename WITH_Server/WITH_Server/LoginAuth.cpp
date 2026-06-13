#include "pch.h"
#include "LoginAuth.h"

#include <limits>

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

LoginAuthCommand::LoginAuthCommand(
	std::string  loginId,
	std::wstring loginIdNormalized,
	std::string  password)
	: _loginId(WidenAscii(loginId))
	, _loginIdNormalized(std::move(loginIdNormalized))
	, _password(std::move(password))
{
}

void LoginAuthCommand::Execute(DBCommandContext& ctx) noexcept
{
	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_AccountLoginAuth(?, ?)}" },
		stmt))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			ctx.LastDBError());
		return;
	}

	if (!stmt.BindString(1, _loginIdNormalized, 64) ||
		!stmt.BindString(2, WidenAscii(_password), 255) ||
		!stmt.Execute() ||
		!stmt.Fetch())
	{
		CompleteQueryError(ctx, stmt);
		return;
	}

	int64_t accountIdRaw{ 0 };
	int32_t resultCode{ -1 };
	int32_t equippedTitleIdRaw{ 0 };
	if (!stmt.GetInt64(1, accountIdRaw) ||
		!stmt.GetInt32(2, resultCode) ||
		!stmt.GetInt32(3, equippedTitleIdRaw) ||
		equippedTitleIdRaw < 0 ||
		equippedTitleIdRaw > static_cast<int32_t>(
			std::numeric_limits<TitleId>::max()))
	{
		ctx.CompleteError(
			static_cast<uint32_t>(DBCommonErrorCode::QueryFail),
			L"sp_AccountLoginAuth: failed to read result set.");
		return;
	}

	LoginAuthPayload payload{};
	switch (resultCode) {
	case 0: // 인증 성공
	{
		payload.accountId = static_cast<uint64_t>(accountIdRaw);
		payload.equippedTitleId =
			static_cast<TitleId>(equippedTitleIdRaw);
		break;
	}
	case 1: // 비밀번호 불일치 또는 Status 비활성
	{
		payload.failReason =
			static_cast<uint32_t>(LoginAuthFailReason::AuthRejected);
		break;
	}
	case 2: // ID 미존재 → 상위에서 회원가입 진행
	{
		payload.failReason =
			static_cast<uint32_t>(LoginAuthFailReason::AccountNotFound);
		payload.loginId = _loginId;
		payload.loginIdNormalized = _loginIdNormalized;
		payload.password = _password;
		break;
	}
	default:
	{
		payload.failReason =
			static_cast<uint32_t>(LoginAuthFailReason::DatabaseError);
		break;
	}
	}

	ctx.CompleteOk(std::move(payload));
}
