#include "pch.h"
#include "DBConn.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace
{
	bool IsSuccess(SQLRETURN rc) noexcept
	{
		return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
	}

	SQLUINTEGER ToSqlUInteger(uint32_t value) noexcept
	{
		return static_cast<SQLUINTEGER>(
			std::min<uint32_t>(value, std::numeric_limits<SQLUINTEGER>::max()));
	}

	void AppendDiagRecords(
		SQLSMALLINT handleType,
		SQLHANDLE handle,
		DBErrorInfo& out) noexcept
	{
		out.Clear();

		for (SQLSMALLINT rec = 1;; ++rec)
		{
			SQLWCHAR state[6] = {};
			SQLINTEGER nativeErr{ 0 };
			SQLWCHAR text[1024] = {};
			SQLSMALLINT textLen{ 0 };

			const SQLRETURN rc = SQLGetDiagRecW(
				handleType,
				handle,
				rec,
				state,
				&nativeErr,
				text,
				static_cast<SQLSMALLINT>(std::size(text)),
				&textLen);

			if (rc == SQL_NO_DATA)
				break;

			if (!IsSuccess(rc))
				break;

			DBDiagRecord diag{};
			diag.sqlState = reinterpret_cast<const wchar_t*>(state);
			diag.nativeError = nativeErr;
			diag.message.assign(
				reinterpret_cast<const wchar_t*>(text),
				static_cast<size_t>(std::max<SQLSMALLINT>(textLen, 0)));
			out.records.push_back(std::move(diag));
		}
	}

	template <typename T>
	std::vector<std::byte> CopyScalarBuffer(const T& value)
	{
		std::vector<std::byte> buffer(sizeof(T));
		std::memcpy(buffer.data(), &value, sizeof(T));
		return buffer;
	}
}

std::wstring DBErrorInfo::Summary() const
{
	std::wstring summary;

	for (const DBDiagRecord& record : records)
	{
		if (!summary.empty())
			summary += L" | ";

		summary += L"[";
		summary += record.sqlState;
		summary += L"] ";
		summary += record.message;
	}

	return summary;
}

bool DBErrorInfo::IsConnectionLost() const noexcept
{
	for (const DBDiagRecord& record : records)
	{
		if (record.sqlState == L"08S01" ||
			record.sqlState == L"08003" ||
			record.sqlState == L"08006")
		{
			return true;
		}
	}

	return false;
}

bool DBErrorInfo::IsTimeout() const noexcept
{
	for (const DBDiagRecord& record : records)
	{
		if (record.sqlState == L"HYT00" ||
			record.sqlState == L"HYT01")
		{
			return true;
		}
	}

	return false;
}

DBStatement::DBStatement(SQLHDBC dbc, uint32_t defaultQueryTimeoutSec)
	: _dbc(dbc)
	, _defaultQueryTimeoutSec(defaultQueryTimeoutSec)
{
}

DBStatement::~DBStatement()
{
	Release();
}

DBStatement::DBStatement(DBStatement&& other) noexcept
{
	MoveFrom(other);
}

DBStatement& DBStatement::operator=(DBStatement&& other) noexcept
{
	if (this != &other)
	{
		Release();
		MoveFrom(other);
	}

	return *this;
}

bool DBStatement::Prepare(std::wstring_view sql)
{
	_lastError.Clear();

	if (_dbc == SQL_NULL_HDBC)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement has no database connection." });
		return false;
	}

	Release();

	SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, _dbc, &_stmt);
	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_DBC, _dbc);
		return false;
	}

	if (_defaultQueryTimeoutSec > 0 && !SetQueryTimeout(_defaultQueryTimeoutSec))
		return false;

	std::wstring sqlCopy{ sql };
	rc = SQLPrepareW(
		_stmt,
		reinterpret_cast<SQLWCHAR*>(sqlCopy.data()),
		SQL_NTS);
	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_STMT, _stmt);
		return false;
	}

	return true;
}

bool DBStatement::Execute()
{
	_lastError.Clear();

	if (_stmt == SQL_NULL_HSTMT)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement is not prepared." });
		return false;
	}

	if (!BindAllParams())
		return false;

	const SQLRETURN rc = SQLExecute(_stmt);
	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_STMT, _stmt);
		return false;
	}

	return true;
}

bool DBStatement::BindInt16(uint16_t index, int16_t value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_SSHORT;
	slot.sqlType = SQL_SMALLINT;
	slot.columnSize = 5;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindInt32(uint16_t index, int32_t value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_SLONG;
	slot.sqlType = SQL_INTEGER;
	slot.columnSize = 10;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindInt64(uint16_t index, int64_t value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_SBIGINT;
	slot.sqlType = SQL_BIGINT;
	slot.columnSize = 19;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindUInt16(uint16_t index, uint16_t value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_USHORT;
	slot.sqlType = SQL_INTEGER;
	slot.columnSize = 10;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindUInt32(uint16_t index, uint32_t value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_ULONG;
	slot.sqlType = SQL_BIGINT;
	slot.columnSize = 19;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindUInt64(uint16_t index, uint64_t value)
{
	if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
	{
		SetLocalError(L"UInt64 parameter exceeds SQL BIGINT range.");
		return false;
	}

	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_UBIGINT;
	slot.sqlType = SQL_BIGINT;
	slot.columnSize = 19;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindBool(uint16_t index, bool value)
{
	const SQLCHAR bitValue = value ? 1 : 0;

	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_BIT;
	slot.sqlType = SQL_BIT;
	slot.columnSize = 1;
	slot.indicator = sizeof(bitValue);
	slot.buffer = CopyScalarBuffer(bitValue);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindFloat(uint16_t index, float value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_FLOAT;
	slot.sqlType = SQL_REAL;
	slot.columnSize = 7;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindDouble(uint16_t index, double value)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_DOUBLE;
	slot.sqlType = SQL_DOUBLE;
	slot.columnSize = 15;
	slot.indicator = sizeof(value);
	slot.buffer = CopyScalarBuffer(value);
	return StoreParam(std::move(slot));
}

bool DBStatement::BindString(
	uint16_t index,
	std::wstring_view value,
	SQLULEN maxChars,
	SQLSMALLINT sqlType)
{
	if (static_cast<SQLULEN>(value.size()) > maxChars)
	{
		SetLocalError(L"String parameter exceeds declared maximum size.");
		return false;
	}

	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_WCHAR;
	slot.sqlType = sqlType;
	slot.columnSize = maxChars;
	slot.indicator = static_cast<SQLLEN>(value.size() * sizeof(wchar_t));

	const size_t byteLen = (value.size() + 1) * sizeof(wchar_t);
	slot.buffer.resize(byteLen);
	std::memcpy(slot.buffer.data(), value.data(), value.size() * sizeof(wchar_t));

	return StoreParam(std::move(slot));
}

bool DBStatement::BindBinary(
	uint16_t index,
	std::span<const std::byte> value,
	SQLULEN maxBytes,
	SQLSMALLINT sqlType)
{
	if (static_cast<SQLULEN>(value.size()) > maxBytes)
	{
		SetLocalError(L"Binary parameter exceeds declared maximum size.");
		return false;
	}

	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_BINARY;
	slot.sqlType = sqlType;
	slot.columnSize = maxBytes;
	slot.indicator = static_cast<SQLLEN>(value.size());
	slot.buffer.assign(value.begin(), value.end());
	if (slot.buffer.empty())
		slot.buffer.resize(1);

	return StoreParam(std::move(slot));
}

bool DBStatement::BindNull(uint16_t index, SQLSMALLINT sqlType)
{
	DBParamSlot slot{};
	slot.index = index;
	slot.cType = SQL_C_DEFAULT;
	slot.sqlType = sqlType;
	slot.indicator = SQL_NULL_DATA;
	return StoreParam(std::move(slot));
}

void DBStatement::ClearParams() noexcept
{
	_params.clear();

	if (_stmt != SQL_NULL_HSTMT)
		SQLFreeStmt(_stmt, SQL_RESET_PARAMS);

	_lastError.Clear();
}

bool DBStatement::Fetch()
{
	_lastError.Clear();

	if (_stmt == SQL_NULL_HSTMT)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement is not prepared." });
		return false;
	}

	const SQLRETURN rc = SQLFetch(_stmt);
	if (rc == SQL_NO_DATA)
		return false;

	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_STMT, _stmt);
		return false;
	}

	return true;
}

void DBStatement::Reset() noexcept
{
	if (_stmt != SQL_NULL_HSTMT)
	{
		SQLFreeStmt(_stmt, SQL_CLOSE);
		SQLFreeStmt(_stmt, SQL_UNBIND);
		SQLFreeStmt(_stmt, SQL_RESET_PARAMS);
	}

	_params.clear();
	_lastError.Clear();
}

void DBStatement::CloseCursor() noexcept
{
	if (_stmt != SQL_NULL_HSTMT)
		SQLCloseCursor(_stmt);
}

bool DBStatement::SetQueryTimeout(uint32_t timeoutSec)
{
	_lastError.Clear();

	if (_stmt == SQL_NULL_HSTMT)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement is not allocated." });
		return false;
	}

	const SQLRETURN rc = SQLSetStmtAttrW(
		_stmt,
		SQL_ATTR_QUERY_TIMEOUT,
		reinterpret_cast<SQLPOINTER>(
			static_cast<uintptr_t>(ToSqlUInteger(timeoutSec))),
		0);

	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_STMT, _stmt);
		return false;
	}

	return true;
}

bool DBStatement::GetString(uint16_t col, std::wstring& out)
{
	_lastError.Clear();
	out.clear();

	if (_stmt == SQL_NULL_HSTMT)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement is not prepared." });
		return false;
	}

	SQLWCHAR buffer[512] = {};
	SQLLEN indicator{ 0 };

	for (;;)
	{
		const SQLRETURN rc = SQLGetData(
			_stmt,
			col,
			SQL_C_WCHAR,
			buffer,
			sizeof(buffer),
			&indicator);

		if (indicator == SQL_NULL_DATA)
		{
			out.clear();
			return true;
		}

		if (rc == SQL_NO_DATA)
			return true;

		if (!(IsSuccess(rc)))
		{
			CaptureDiag(SQL_HANDLE_STMT, _stmt);
			return false;
		}

		const size_t chunkLen = wcsnlen_s(
			reinterpret_cast<const wchar_t*>(buffer),
			std::size(buffer));
		out.append(reinterpret_cast<const wchar_t*>(buffer), chunkLen);

		if (rc == SQL_SUCCESS)
			return true;
	}
}

bool DBStatement::GetInt32(uint16_t col, int32_t& out)
{
	_lastError.Clear();
	out = 0;

	if (_stmt == SQL_NULL_HSTMT)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement is not prepared." });
		return false;
	}

	SQLINTEGER value{ 0 };
	SQLLEN indicator{ 0 };
	const SQLRETURN rc = SQLGetData(
		_stmt,
		col,
		SQL_C_SLONG,
		&value,
		sizeof(value),
		&indicator);

	if (!(IsSuccess(rc)))
	{
		CaptureDiag(SQL_HANDLE_STMT, _stmt);
		return false;
	}

	if (indicator == SQL_NULL_DATA)
	{
		SetLocalError(L"Integer column is NULL.");
		return false;
	}

	out = static_cast<int32_t>(value);
	return true;
}

bool DBStatement::GetInt64(uint16_t col, int64_t& out)
{
	_lastError.Clear();
	out = 0;

	if (_stmt == SQL_NULL_HSTMT)
	{
		_lastError.records.push_back(
			DBDiagRecord{ L"", 0, L"Statement is not prepared." });
		return false;
	}

	SQLBIGINT value{ 0 };
	SQLLEN indicator{ 0 };
	const SQLRETURN rc = SQLGetData(
		_stmt,
		col,
		SQL_C_SBIGINT,
		&value,
		sizeof(value),
		&indicator);

	if (!(IsSuccess(rc)))
	{
		CaptureDiag(SQL_HANDLE_STMT, _stmt);
		return false;
	}

	if (indicator == SQL_NULL_DATA)
	{
		SetLocalError(L"Bigint column is NULL.");
		return false;
	}

	out = static_cast<int64_t>(value);
	return true;
}

void DBStatement::Release() noexcept
{
	if (_stmt != SQL_NULL_HSTMT)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, _stmt);
		_stmt = SQL_NULL_HSTMT;
	}

	_params.clear();
}

void DBStatement::MoveFrom(DBStatement& other) noexcept
{
	_dbc = other._dbc;
	_stmt = other._stmt;
	_defaultQueryTimeoutSec = other._defaultQueryTimeoutSec;
	_lastError = std::move(other._lastError);
	_params = std::move(other._params);

	other._dbc = SQL_NULL_HDBC;
	other._stmt = SQL_NULL_HSTMT;
	other._defaultQueryTimeoutSec = 0;
	other._lastError.Clear();
	other._params.clear();
}

void DBStatement::CaptureDiag(SQLSMALLINT handleType, SQLHANDLE handle) noexcept
{
	AppendDiagRecords(handleType, handle, _lastError);
}

void DBStatement::SetLocalError(std::wstring message)
{
	_lastError.Clear();
	_lastError.records.push_back(DBDiagRecord{ L"", 0, std::move(message) });
}

bool DBStatement::StoreParam(DBParamSlot slot)
{
	_lastError.Clear();

	if (_stmt == SQL_NULL_HSTMT)
	{
		SetLocalError(L"Statement is not prepared.");
		return false;
	}

	if (slot.index == 0)
	{
		SetLocalError(L"ODBC parameter index starts at 1.");
		return false;
	}

	auto it = std::find_if(
		_params.begin(),
		_params.end(),
		[slotIndex = slot.index](const DBParamSlot& existing)
		{
			return existing.index == slotIndex;
		});

	if (it != _params.end())
		*it = std::move(slot);
	else
		_params.push_back(std::move(slot));

	return true;
}

bool DBStatement::BindAllParams()
{
	if (_stmt == SQL_NULL_HSTMT)
	{
		SetLocalError(L"Statement is not prepared.");
		return false;
	}

	if (_params.empty())
		return true;

	SQLFreeStmt(_stmt, SQL_RESET_PARAMS);

	for (DBParamSlot& slot : _params)
	{
		SQLPOINTER valuePtr = nullptr;
		SQLLEN bufferLen = 0;

		if (slot.indicator != SQL_NULL_DATA)
		{
			valuePtr = slot.buffer.data();
			bufferLen = static_cast<SQLLEN>(slot.buffer.size());
		}

		const SQLRETURN rc = SQLBindParameter(
			_stmt,
			slot.index,
			SQL_PARAM_INPUT,
			slot.cType,
			slot.sqlType,
			slot.columnSize,
			slot.decimalDigits,
			valuePtr,
			bufferLen,
			&slot.indicator);

		if (!IsSuccess(rc))
		{
			CaptureDiag(SQL_HANDLE_STMT, _stmt);
			return false;
		}
	}

	return true;
}

DBConn::DBConn() = default;

DBConn::~DBConn()
{
	Disconnect();
	FreeHandles();
}

bool DBConn::ConnectDSN(
	std::wstring_view dsn,
	std::wstring_view user,
	std::wstring_view password,
	const DBConnConfig& config)
{
	return ConnectInternal(dsn, true, user, password, config);
}

bool DBConn::ConnectConnStr(
	std::wstring_view connStr,
	const DBConnConfig& config)
{
	return ConnectInternal(connStr, false, L"", L"", config);
}

void DBConn::Disconnect() noexcept
{
	if (!_connected)
		return;

	SQLDisconnect(_hDbc);
	_connected = false;
}

bool DBConn::Prepare(DBQueryText sql, DBStatement& outStmt)
{
	_lastError.Clear();

	if (!_connected)
	{
		SetLocalError(L"Not connected.");
		return false;
	}

	DBStatement stmt{ _hDbc, _defaultQueryTimeoutSec };
	if (!stmt.Prepare(sql.View()))
	{
		_lastError = stmt.LastErrorInfo();
		return false;
	}

	outStmt = std::move(stmt);
	return true;
}

bool DBConn::ExecuteNonQuery(DBQueryText sql)
{
	_lastError.Clear();

	DBStatement stmt;
	if (!Prepare(sql, stmt))
		return false;

	if (!stmt.Execute())
	{
		_lastError = stmt.LastErrorInfo();
		return false;
	}

	return true;
}

bool DBConn::ExecuteScalar(DBQueryText sql, std::wstring& outFirstCol)
{
	_lastError.Clear();
	outFirstCol.clear();

	DBStatement stmt;
	if (!Prepare(sql, stmt))
		return false;

	if (!stmt.Execute())
	{
		_lastError = stmt.LastErrorInfo();
		return false;
	}

	if (!stmt.Fetch())
	{
		if (!stmt.LastErrorInfo().Empty())
		{
			_lastError = stmt.LastErrorInfo();
			return false;
		}

		return true;
	}

	if (!stmt.GetString(1, outFirstCol))
	{
		_lastError = stmt.LastErrorInfo();
		return false;
	}

	return true;
}

bool DBConn::AllocHandles()
{
	if (_hEnv != SQL_NULL_HENV)
		return true;

	SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv);
	if (!IsSuccess(rc))
		return false;

	rc = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_ENV, _hEnv);
		return false;
	}

	rc = SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc);
	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_ENV, _hEnv);
		return false;
	}

	return true;
}

void DBConn::FreeHandles() noexcept
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

bool DBConn::ConnectInternal(
	std::wstring_view connStrOrDsn,
	bool useDsn,
	std::wstring_view user,
	std::wstring_view password,
	const DBConnConfig& config)
{
	_lastError.Clear();

	if (!AllocHandles())
	{
		if (_lastError.Empty())
			SetLocalError(L"ODBC handle allocation failed.");
		return false;
	}

	if (_connected)
		return true;

	if (config.loginTimeoutSec > 0)
	{
		const SQLRETURN timeoutRc = SQLSetConnectAttrW(
			_hDbc,
			SQL_LOGIN_TIMEOUT,
			reinterpret_cast<SQLPOINTER>(
				static_cast<uintptr_t>(ToSqlUInteger(config.loginTimeoutSec))),
			0);
		if (!IsSuccess(timeoutRc))
		{
			CaptureDiag(SQL_HANDLE_DBC, _hDbc);
			return false;
		}
	}

	const SQLUINTEGER autoCommit =
		config.autoCommit ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
	const SQLRETURN autoCommitRc = SQLSetConnectAttrW(
		_hDbc,
		SQL_ATTR_AUTOCOMMIT,
		reinterpret_cast<SQLPOINTER>(static_cast<uintptr_t>(autoCommit)),
		0);
	if (!IsSuccess(autoCommitRc))
	{
		CaptureDiag(SQL_HANDLE_DBC, _hDbc);
		return false;
	}

	SQLRETURN rc{ SQL_ERROR };
	const std::wstring connStrOrDsnCopy{ connStrOrDsn };
	if (useDsn)
	{
		const std::wstring userCopy{ user };
		const std::wstring passwordCopy{ password };

		rc = SQLConnectW(
			_hDbc,
			reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(connStrOrDsnCopy.c_str())),
			SQL_NTS,
			reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(userCopy.c_str())),
			SQL_NTS,
			reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(passwordCopy.c_str())),
			SQL_NTS);
	}
	else
	{
		SQLWCHAR outConn[2048] = {};
		SQLSMALLINT outLen{ 0 };

		rc = SQLDriverConnectW(
			_hDbc,
			nullptr,
			reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(connStrOrDsnCopy.c_str())),
			SQL_NTS,
			outConn,
			static_cast<SQLSMALLINT>(std::size(outConn)),
			&outLen,
			SQL_DRIVER_NOPROMPT);
	}

	if (!IsSuccess(rc))
	{
		CaptureDiag(SQL_HANDLE_DBC, _hDbc);
		return false;
	}

	_connected = true;
	_defaultQueryTimeoutSec = config.defaultQueryTimeoutSec;
	return true;
}

void DBConn::CaptureDiag(SQLSMALLINT handleType, SQLHANDLE handle) noexcept
{
	AppendDiagRecords(handleType, handle, _lastError);
}

void DBConn::SetLocalError(std::wstring message)
{
	_lastError.Clear();
	_lastError.records.push_back(DBDiagRecord{ L"", 0, std::move(message) });
}
