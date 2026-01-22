#pragma once

#include <sqlext.h>

struct DBQuery {

};

class DBManager {
public:
	static DBManager& Get()
	{
		static DBManager instance;
		return instance;
	}

	~DBManager();

	void Start(std::wstring_view database);
	void Stop();

	void QueryRequest(DBQuery query);

private:
	DBManager();

	bool EnsureThreadConnection();

	bool _running;

	SQLHENV _hEnv;
	SQLHDBC _hDbc;

	std::wstring _database;
	std::thread _worker;

	concurrency::concurrent_queue<DBQuery> _queryQueue;
};

