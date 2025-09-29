#include "pch.h"
#include "DBManager.h"

MSSQLManager::MSSQLManager(const std::wstring& database) : IDBManager(database)
{
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv);
	if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
		LOG_ERR("SQLHENV Alloc Failed");
		return;
	}

	retcode = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
		LOG_ERR("SQLHENV Set Attr Failed");
		return;
	}
	
	LOG_DBG("DBManager Construct Success");
}

MSSQLManager::~MSSQLManager()
{
	if (_hDbc != SQL_NULL_HDBC) {
		SQLDisconnect(_hDbc);
		SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
	}

	if (_hEnv != SQL_NULL_HENV) {
		SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
	}

	LOG_DBG("DBManager Shutdown Success");
}

bool MSSQLManager::ExecuteQuery(const std::wstring& query, const std::function<bool(SQLHSTMT)>& binder)
{
	if (EnsureThreadConnection()) {
		SQLHSTMT hStmt{ SQL_NULL_HSTMT };
		SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &hStmt);
		if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
			LOG_ERR("SQLHSTMT Alloc Failed");
			return false;
		}

		retcode = SQLExecDirect(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
		if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
			LOG_ERR("SQLExecDirect Failed");
			SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
			return false;
		}
		
		bool result{ true };
		if (binder) {
			result = binder(hStmt);
		}

		SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		return result;
	}

	return false;
}

bool MSSQLManager::EnsureThreadConnection()
{
	if (_hDbc == SQL_NULL_HDBC) {
		SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc);
		if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
			LOG_ERR("SQLHDBC Alloc Failed");
			return false;
		}

		retcode = SQLConnect(_hDbc, (SQLWCHAR*)_database.c_str(), SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
		if ((retcode != SQL_SUCCESS) and (retcode != SQL_SUCCESS_WITH_INFO)) {
			LOG_ERR("SQLConnect Failed");
			return false;
		}
	}

	return true;
}
