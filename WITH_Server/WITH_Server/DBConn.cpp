#include "pch.h"
#include "DBConn.h"

DBConn::DBConn()
	: _hEnv(SQL_NULL_HENV), _hDbc(SQL_NULL_HDBC), _connected(false)
{
}

DBConn::~DBConn()
{
	Disconnect();
	FreeHandles();
}

bool DBConn::ConnectDSN(std::wstring_view dsn, std::wstring_view user, std::wstring_view password)
{
	return ConnectInternal(dsn, true, user, password);
}

bool DBConn::ConnectConnStr(std::wstring_view connStr)
{
	return ConnectInternal(connStr, false, L"", L"");
}

void DBConn::Disconnect()
{
	if (!_connected) return;

	SQLDisconnect(_hDbc);
	_connected = false;
}

bool DBConn::ExecuteNonQuery(std::wstring_view sql)
{
	_lastError.clear();

	if (!_connected)
	{
		_lastError = L"Not connected.";
		return false;
	}

	SQLHSTMT stmt{ SQL_NULL_HSTMT };
	if (SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &stmt) != SQL_SUCCESS)
	{
		_lastError = ExtractDiag(SQL_HANDLE_DBC, _hDbc);
		return false;
	}

	SQLRETURN rc = SQLExecDirectW(stmt, (SQLWCHAR*)sql.data(), SQL_NTS);
	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
	{
		_lastError = ExtractDiag(SQL_HANDLE_STMT, stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return false;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	return true;
}

bool DBConn::ExecuteScalar(std::wstring_view sql, std::wstring& outFirstCol)
{
	_lastError.clear();
	outFirstCol.clear();

	if (!_connected)
	{
		_lastError = L"Not connected.";
		return false;
	}

	SQLHSTMT stmt{ SQL_NULL_HSTMT };
	if (SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &stmt) != SQL_SUCCESS)
	{
		_lastError = ExtractDiag(SQL_HANDLE_DBC, _hDbc);
		return false;
	}

	SQLRETURN rc = SQLExecDirectW(stmt, (SQLWCHAR*)sql.data(), SQL_NTS);
	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
	{
		_lastError = ExtractDiag(SQL_HANDLE_STMT, stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return false;
	}

	rc = SQLFetch(stmt);
	if (rc == SQL_NO_DATA)
	{
		outFirstCol = L"";
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return true;
	}

	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
	{
		_lastError = ExtractDiag(SQL_HANDLE_STMT, stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return false;
	}

	SQLWCHAR buf[1024] = {};
	SQLLEN indicator{ 0 };
	rc = SQLGetData(stmt, 1, SQL_C_WCHAR, buf, sizeof(buf), &indicator);
	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
	{
		_lastError = ExtractDiag(SQL_HANDLE_STMT, stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return false;
	}

	if (indicator == SQL_NULL_DATA) outFirstCol = L"";
	else outFirstCol = buf;

	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	return true;
}

bool DBConn::AllocHandles()
{
	if (_hEnv != SQL_NULL_HENV) return true;

	SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv);
	if(!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
		return false;

	rc = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
		return false;

	rc = SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc);
	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
		return false;

	return true;
}

void DBConn::FreeHandles()
{
	if (_hDbc != SQL_NULL_HDBC)
	{
		SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
		_hDbc = SQL_NULL_HDBC;
	}

	if (_hEnv != SQL_NULL_HENV)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
		_hEnv = SQL_NULL_HENV;
	}
}

bool DBConn::ConnectInternal(std::wstring_view connStrOrDsn, bool useDsn, std::wstring_view user, std::wstring_view password)
{
	_lastError.clear();

	if (!AllocHandles())
	{
		_lastError = L"Allochandles failed.";
		return false;
	}

	if (_connected) return true;

	SQLRETURN rc{ SQL_ERROR };
	if (useDsn)
	{
		SQLSMALLINT connStrOrDsnSize = static_cast<SQLSMALLINT>(connStrOrDsn.size());

		rc = SQLConnectW(_hDbc,
			(SQLWCHAR*)connStrOrDsn.data(), connStrOrDsnSize,
			(SQLWCHAR*)user.data(), connStrOrDsnSize,
			(SQLWCHAR*)password.data(), connStrOrDsnSize
		);
	}

	else
	{
		SQLWCHAR outConn[2048] = {};
		SQLSMALLINT outLen{ 0 };

		rc = SQLDriverConnectW(
			_hDbc, nullptr,
			(SQLWCHAR*)connStrOrDsn.data(), (SQLSMALLINT)connStrOrDsn.size(),
			outConn, 2048,
			&outLen, SQL_DRIVER_NOPROMPT
		);
	}

	if (!(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
	{
		_lastError = ExtractDiag(SQL_HANDLE_DBC, _hDbc);
		return false;
	}

	_connected = true;
	return true;
}

std::wstring DBConn::ExtractDiag(SQLSMALLINT handleType, SQLHANDLE handle) const
{
	std::wstring msg;
	SQLWCHAR state[6] = { };
	SQLINTEGER nativeErr{ 0 };
	SQLWCHAR text[1024] = { };
	SQLSMALLINT textLen{ 0 };

	SQLRETURN rc = SQLGetDiagRecW(handleType, handle, 1, state, &nativeErr, text, 1024, &textLen);

	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
	{
		msg = L"[";
		msg += state;
		msg += L"]";
		msg += text;
	}

	return msg;
}
