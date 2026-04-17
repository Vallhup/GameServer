#pragma once

enum class DBOp : uint8_t
{
	None,
	Login,
	LoadTitles,
	GrantTitle,
	EquipTitle,

	Test,
};

enum class DBError : uint16_t
{
	None,
	ConnectFail,
	QueryFail,

	AccountNotFound,
	InvalidPassword,

	TitleNotOwned,
	InvalidTitle,
};

struct DBResult
{
	DBOp op{ DBOp::None };
	uint64_t requestId{ 0 };

	bool ok{ false };
	DBError error{ DBError::None };

	// 로그용
	std::wstring msg;

	/*union {
		struct 
		{
			uint64_t accountId{ 0 };
			uint32_t sessionVersion{ 0 };
		} DBLoginPayload;

		struct
		{
			std::vector<uint32_t> ownedTitleIds;
			uint32_t equippedTitleId{ 0 };
		} DBTitlesPayload;
	};*/
};

struct IDBCommand 
{
	DBOp op{ DBOp::None };
	uint64_t requestId{ 0 };

	virtual ~IDBCommand() = default;
	virtual void Execute(class DBConn& conn, class DBManager& manager) = 0;
};

struct TestCommand : public IDBCommand {
	TestCommand(uint64_t rid);
	virtual void Execute(DBConn& conn, DBManager& manager) override;
};