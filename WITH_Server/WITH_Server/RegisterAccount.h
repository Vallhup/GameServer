#pragma once

#include <cstdint>
#include <string>

#include "DBData.h"

constexpr DBCommandTypeId kRegisterAccountCommandTypeId = 2;
constexpr DBPayloadTypeId kRegisterAccountPayloadTypeId = 2;

enum class RegisterAccountFailReason : uint32_t
{
	None                = 0,
	DuplicateId         = 1, // 동시 요청으로 같은 ID 선점 (UNIQUE 제약 위반)
	DatabaseUnavailable = 2,
	DatabaseError       = 3,
};

struct RegisterAccountPayload
{
	static constexpr DBPayloadTypeId PayloadTypeId = kRegisterAccountPayloadTypeId;

	uint64_t accountId{ 0 };
	uint32_t failReason{ 0 };
};

class RegisterAccountCommand final : public IDBCommand
{
public:
	RegisterAccountCommand(
		std::wstring loginId,
		std::wstring loginIdNormalized,
		std::string  password);

	DBCommandTypeId DebugTypeId() const noexcept override
	{
		return kRegisterAccountCommandTypeId;
	}

	const char* DebugName() const noexcept override
	{
		return "RegisterAccount";
	}

	void Execute(DBCommandContext& ctx) noexcept override;

private:
	std::wstring _loginId;
	std::wstring _loginIdNormalized;
	std::string  _password;
};
