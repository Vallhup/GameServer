#include "pch.h"
#include "DBData.h"
#include "DBConn.h"
#include "DBManager.h"

TestCommand::TestCommand(uint64_t rid)
{
	op = DBOp::Test;
	requestId = rid;
}

void TestCommand::Execute(DBConn& conn, DBManager& manager)
{
	DBResult result;
	result.op = op;
	result.requestId = requestId;

	std::wstring out;
	if (!conn.ExecuteScalar(L"SELECT 1", out))
	{
		result.ok = false;
		result.error = DBError::QueryFail;
		result.msg = conn.LastError();
	}

	else
	{
		result.ok = true;
		result.error = DBError::None;
		result.msg = L"OK";
	}

	manager.PushResult(std::move(result));
}
