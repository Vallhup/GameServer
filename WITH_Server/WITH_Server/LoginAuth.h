#pragma once

#include <cstdint>
#include <string>

#include "DBData.h"

constexpr DBCommandTypeId kLoginAuthCommandTypeId = 1;
constexpr DBPayloadTypeId kLoginAuthPayloadTypeId = 1;

enum class LoginAuthFailReason : uint32_t
{
	None                = 0,
	MalformedPacket     = 1,
	DuplicateLogin      = 2,
	EntryStartFailed    = 3,
	AuthRejected        = 4,
	DatabaseUnavailable = 5,
	DatabaseError       = 6,
	Timeout             = 7,
	AccountNotFound     = 8,
};

struct LoginAuthPayload
{
	static constexpr DBPayloadTypeId PayloadTypeId = kLoginAuthPayloadTypeId;

	uint64_t     accountId{ 0 };
	uint32_t     failReason{ 0 };

	// AccountNotFound 시 RegisterAccountCommand에 전달할 자격증명
	std::wstring loginId;
	std::wstring loginIdNormalized;
	std::string  password;
};

class LoginAuthCommand final : public IDBCommand
{
public:
	LoginAuthCommand(
		std::string  loginId,
		std::wstring loginIdNormalized,
		std::string  password);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kLoginAuthCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "LoginAuth";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	std::wstring _loginId;
	std::wstring _loginIdNormalized;
	std::string  _password;
};
