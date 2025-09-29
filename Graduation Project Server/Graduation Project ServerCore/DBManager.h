#pragma once

// DB 연동을 위한 기반 코드를 위한 Manager
// (ODBC API를 사용하기 위한 환경 변수 초기화, Statement Setting 등)
// 
// ODBC 자체는 Thread-Safe를 전제로 설계
// but 같은 핸들(HENV/HDBC/HSTMT)을 여러 스레드가 동시에 건드릴 시 
// 내부적으로 직렬화(= 성능 저하) 혹은 제한하는 경우가 있음
// 
// Connection Pool로 Thread-Safe 지원

class IDBManager {
public:
	IDBManager() = delete;
	virtual ~IDBManager() = default;

public:
	virtual bool ExecuteQuery(const std::wstring& query, 
		const std::function<bool(SQLHSTMT)>& binder) = 0;

protected:
	IDBManager(const std::wstring& database) : _database(database) {}

protected:
	std::wstring _database;
};

class MSSQLManager : public IDBManager {
public:
	MSSQLManager() = delete;
	MSSQLManager(const std::wstring& database);
	~MSSQLManager();

public:
	virtual bool ExecuteQuery(const std::wstring& query, 
		const std::function<bool(SQLHSTMT)>& binder) override;

private:
	bool EnsureThreadConnection();

private:
	SQLHENV _hEnv;
	inline static thread_local SQLHDBC _hDbc;
};