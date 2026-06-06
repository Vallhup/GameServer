#pragma once

#include <sqlext.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct DBDiagRecord
{
	std::wstring sqlState;
	SQLINTEGER nativeError{ 0 };
	std::wstring message;
};

struct DBErrorInfo
{
	std::vector<DBDiagRecord> records;

	void Clear() noexcept { records.clear(); }

	[[nodiscard]]
	bool Empty() const noexcept { return records.empty(); }

	[[nodiscard]]
	std::wstring Summary() const;

	[[nodiscard]]
	bool IsConnectionLost() const noexcept;

	[[nodiscard]]
	bool IsTimeout() const noexcept;
};

struct DBConnConfig
{
	uint32_t loginTimeoutSec{ 3 };
	uint32_t defaultQueryTimeoutSec{ 3 };
	bool autoCommit{ true };
};

class DBQueryText
{
public:
	template<size_t N>
	consteval DBQueryText(const wchar_t (&sql)[N]) noexcept
		: _sql(sql)
		, _length(N > 0 ? N - 1 : 0)
	{
	}

	DBQueryText(std::wstring_view) = delete;
	DBQueryText(const std::wstring&) = delete;

	[[nodiscard]]
	const wchar_t* CStr() const noexcept { return _sql; }

	[[nodiscard]]
	size_t Length() const noexcept { return _length; }

	[[nodiscard]]
	std::wstring_view View() const noexcept { return { _sql, _length }; }

private:
	const wchar_t* _sql{ L"" };
	size_t _length{ 0 };
};

class DBStatement
{
public:
	DBStatement() = default;
	~DBStatement();

	DBStatement(const DBStatement&) = delete;
	DBStatement& operator=(const DBStatement&) = delete;

	DBStatement(DBStatement&& other) noexcept;
	DBStatement& operator=(DBStatement&& other) noexcept;

	[[nodiscard]]
	bool IsValid() const noexcept { return _stmt != SQL_NULL_HSTMT; }

	[[nodiscard]]
	bool Execute();

	[[nodiscard]]
	bool BindInt16(uint16_t index, int16_t value);

	[[nodiscard]]
	bool BindInt32(uint16_t index, int32_t value);

	[[nodiscard]]
	bool BindInt64(uint16_t index, int64_t value);

	[[nodiscard]]
	bool BindUInt16(uint16_t index, uint16_t value);

	[[nodiscard]]
	bool BindUInt32(uint16_t index, uint32_t value);

	[[nodiscard]]
	bool BindUInt64(uint16_t index, uint64_t value);

	[[nodiscard]]
	bool BindBool(uint16_t index, bool value);

	[[nodiscard]]
	bool BindFloat(uint16_t index, float value);

	[[nodiscard]]
	bool BindDouble(uint16_t index, double value);

	[[nodiscard]]
	bool BindString(
		uint16_t index,
		std::wstring_view value,
		SQLULEN maxChars,
		SQLSMALLINT sqlType = SQL_WVARCHAR);

	[[nodiscard]]
	bool BindBinary(
		uint16_t index,
		std::span<const std::byte> value,
		SQLULEN maxBytes,
		SQLSMALLINT sqlType = SQL_VARBINARY);

	[[nodiscard]]
	bool BindNull(uint16_t index, SQLSMALLINT sqlType);

	void ClearParams() noexcept;

	// Returns true when a row is available. Returns false for SQL_NO_DATA or error.
	// Use LastErrorInfo().Empty() to distinguish SQL_NO_DATA from an error.
	[[nodiscard]]
	bool Fetch();

	// Advances to the next result set. Returns false for SQL_NO_DATA or error.
	// Use LastErrorInfo().Empty() to distinguish SQL_NO_DATA from an error.
	[[nodiscard]]
	bool MoreResults();

	void Reset() noexcept;
	void CloseCursor() noexcept;

	[[nodiscard]]
	bool SetQueryTimeout(uint32_t timeoutSec);

	[[nodiscard]]
	bool GetString(uint16_t col, std::wstring& out);

	[[nodiscard]]
	bool GetInt32(uint16_t col, int32_t& out);

	[[nodiscard]]
	bool GetInt64(uint16_t col, int64_t& out);

	[[nodiscard]]
	const DBErrorInfo& LastErrorInfo() const noexcept { return _lastError; }

	[[nodiscard]]
	std::wstring LastError() const { return _lastError.Summary(); }

private:
	friend class DBConn;

	DBStatement(SQLHDBC dbc, uint32_t defaultQueryTimeoutSec);

	[[nodiscard]]
	bool Prepare(std::wstring_view sql);

	struct DBParamSlot
	{
		SQLUSMALLINT index{ 0 };
		SQLSMALLINT cType{ SQL_C_DEFAULT };
		SQLSMALLINT sqlType{ SQL_VARCHAR };
		SQLULEN columnSize{ 0 };
		SQLSMALLINT decimalDigits{ 0 };
		SQLLEN indicator{ 0 };
		std::vector<std::byte> buffer;
	};

	void Release() noexcept;
	void MoveFrom(DBStatement& other) noexcept;
	void CaptureDiag(SQLSMALLINT handleType, SQLHANDLE handle) noexcept;
	void SetLocalError(std::wstring message);

	[[nodiscard]]
	bool StoreParam(DBParamSlot slot);

	[[nodiscard]]
	bool BindAllParams();

private:
	SQLHDBC		_dbc{ SQL_NULL_HDBC };
	SQLHSTMT	_stmt{ SQL_NULL_HSTMT };
	uint32_t	_defaultQueryTimeoutSec{ 0 };
	DBErrorInfo _lastError;
	std::vector<DBParamSlot> _params;
};

class DBConn
{
public:
	DBConn();
	~DBConn();

	DBConn(const DBConn&) = delete;
	DBConn& operator=(const DBConn&) = delete;

	[[nodiscard]]
	bool ConnectDSN(
		std::wstring_view dsn,
		std::wstring_view user = L"",
		std::wstring_view password = L"",
		const DBConnConfig& config = {});

	[[nodiscard]]
	bool ConnectConnStr(
		std::wstring_view connStr,
		const DBConnConfig& config = {});

	void Disconnect() noexcept;

	[[nodiscard]]
	bool IsConnected() const noexcept { return _connected; }

	[[nodiscard]]
	bool Prepare(DBQueryText sql, DBStatement& outStmt);

	[[nodiscard]]
	bool ExecuteNonQuery(DBQueryText sql);

	[[nodiscard]]
	bool ExecuteScalar(DBQueryText sql, std::wstring& outFirstCol);

	[[nodiscard]]
	const DBErrorInfo& LastErrorInfo() const noexcept { return _lastError; }

	[[nodiscard]]
	std::wstring LastError() const { return _lastError.Summary(); }

	[[nodiscard]]
	std::wstring LastErrorSummary() const { return _lastError.Summary(); }

private:
	[[nodiscard]]
	bool AllocHandles();

	void FreeHandles() noexcept;

	[[nodiscard]]
	bool ConnectInternal(
		std::wstring_view connStrOrDsn,
		bool useDsn,
		std::wstring_view user,
		std::wstring_view password,
		const DBConnConfig& config);

	void CaptureDiag(SQLSMALLINT handleType, SQLHANDLE handle) noexcept;
	void SetLocalError(std::wstring message);

private:
	SQLHENV _hEnv{ SQL_NULL_HENV };
	SQLHDBC _hDbc{ SQL_NULL_HDBC };

	bool _connected{ false };
	uint32_t _defaultQueryTimeoutSec{ 0 };
	DBErrorInfo _lastError;
};
