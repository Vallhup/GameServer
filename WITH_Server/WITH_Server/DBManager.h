#pragma once

#include <sqlext.h>
#include "DBData.h"

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

	void PushCommand(std::shared_ptr<IDBCommand> cmd);
	void PushResult(DBResult&& result);
	bool TryPopResult(DBResult& out);

private:
	DBManager();
	void WorkerLoop();

	bool _running;

	std::wstring _database;
	std::thread _worker;

	concurrency::concurrent_queue<std::shared_ptr<IDBCommand>> _commandQueue;
	concurrency::concurrent_queue<DBResult> _resultQueue;
};

