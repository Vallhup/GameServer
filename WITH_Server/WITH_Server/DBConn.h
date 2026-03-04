#pragma once

#include <sqlext.h>

class DBConn {
public:
	DBConn();
	~DBConn();

	DBConn(const DBConn&) = delete;
	DBConn& operator=(const DBConn&) = delete;

	bool ConnectDSN(std::wstring_view dsn,
		std::wstring_view user = L"",
		std::wstring_view password = L"");
	bool ConnectConnStr(std::wstring_view connStr);

	void Disconnect();
	bool IsConnected() const { return _connected; }

	bool ExecuteNonQuery(std::wstring_view sql);
	bool ExecuteScalar(std::wstring_view sql, std::wstring& outFirstCol);

	std::wstring LastError() const { return _lastError; }

private:
	bool AllocHandles();
	void FreeHandles();

	bool ConnectInternal(std::wstring_view connStrOrDsn, bool useDsn,
		std::wstring_view user, std::wstring_view password);

	std::wstring ExtractDiag(SQLSMALLINT handleType, SQLHANDLE handle) const;


	SQLHENV _hEnv;
	SQLHDBC _hDbc;

	bool _connected;
	mutable std::wstring _lastError;
};

