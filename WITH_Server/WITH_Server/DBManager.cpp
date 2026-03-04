#include "pch.h"
#include "DBManager.h"
#include "DBConn.h"

DBManager::DBManager() : _running(false)
{
}

DBManager::~DBManager()
{
}

void DBManager::Start(std::wstring_view database)
{
	using namespace std::chrono;

	_running = true;
	_database = database;
	_worker = std::thread([this]() { this->WorkerLoop(); });
}

void DBManager::Stop()
{
	_running = false;
	_worker.join();
}

void DBManager::PushCommand(std::shared_ptr<IDBCommand> cmd)
{
	_commandQueue.push(std::move(cmd));
}

void DBManager::PushResult(DBResult&& result)
{
	_resultQueue.push(std::move(result));
}

bool DBManager::TryPopResult(DBResult& out)
{
	return _resultQueue.try_pop(out);
}

void DBManager::WorkerLoop()
{
	DBConn conn;
	if (!conn.ConnectDSN(_database))
	{
		DBResult result;
		result.op = DBOp::Test;
		result.requestId = 0;
		result.ok = false;
		result.error = DBError::ConnectFail;
		result.msg = L"Connect failed.";

		PushResult(std::move(result));
		return;
	}

	while (_running)
	{
		std::shared_ptr<IDBCommand> cmd;
		if (_commandQueue.try_pop(cmd))
		{
			cmd->Execute(conn, *this);
		}
	}

	conn.Disconnect();
}
