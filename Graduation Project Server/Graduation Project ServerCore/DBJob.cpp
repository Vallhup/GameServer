#include "pch.h"
#include "DBJob.h"

/*---------------[ LoginJob ]---------------*/

LoginJob::LoginJob(IGameContext& gameCtx, std::wstring_view id, std::wstring_view password, Session* session)
	: DBJob(gameCtx), _id(id), _password(password), _session(session)
{
}

void LoginJob::Execute()
{
	// TEMP : 나중에 DB Table, Stored Procedure 구현 필요
	std::wstring query = L"EXEC user_login " + _id;
	auto binder = [](SQLHSTMT hStmt) -> bool
		{
			SQLWCHAR userId;
			SQLLEN cbUserId;

			SQLRETURN retcode = SQLBindCol(hStmt, 1, SQL_C_WCHAR, &userId, 20, &cbUserId);
			if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
				LOG_ERR("SQLBindCol Failed");
				return false;
			}

			retcode = SQLFetch(hStmt);
			if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
				LOG_ERR("SQLFetch Failed");
				return false;
			}

			return true;
		};

	_gameCtx.GetDBManager().ExecuteQuery(query, binder);
}