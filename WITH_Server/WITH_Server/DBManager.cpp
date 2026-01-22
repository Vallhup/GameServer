#include "pch.h"
#include "DBManager.h"

DBManager::DBManager() : _running(false)
{
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, 
		SQL_NULL_HANDLE, &_hEnv);

	if ((retcode != SQL_SUCCESS) &&
		(retcode != SQL_SUCCESS_WITH_INFO))
	{
#ifdef _DEBUG
		std::cout << "SQLHENV Alloc Failed\n";
#endif
		return;
	}

	retcode = SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION,
		(SQLPOINTER)SQL_OV_ODBC3, 0);

	if ((retcode != SQL_SUCCESS) &&
		(retcode != SQL_SUCCESS_WITH_INFO))
	{
#ifdef _DEBUG
		std::cout << "SQLHENV Set Attr Failed\n";
#endif
		return;
	}
}

DBManager::~DBManager()
{
	if (_hDbc != SQL_NULL_HDBC)
	{
		SQLDisconnect(_hDbc);
		SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
	}

	if (_hEnv != SQL_NULL_HENV)
		SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
}

void DBManager::Start(std::wstring_view database)
{
	using namespace std::chrono;

	_running = true;
	_database = database;
	_worker = std::thread(
		[&]()
		{
			while (_running)
			{
				DBQuery query;
				if (_queryQueue.try_pop(query))
				{
					// TODO : Query ½ÇÇà
				}

				std::this_thread::sleep_for(1ms);
			}
		}
	);

	_worker.join();
}

void DBManager::Stop()
{
	_running = false;
}

void DBManager::QueryRequest(DBQuery query)
{
	_queryQueue.push(query);
}

bool DBManager::EnsureThreadConnection()
{
	if (_hDbc == SQL_NULL_HDBC)
	{
		SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_DBC,
			_hEnv, &_hDbc);

		if ((retcode != SQL_SUCCESS) &&
			(retcode != SQL_SUCCESS_WITH_INFO))
		{
#ifdef _DEBUG
			std::cout << "SQLHDBC Alloc Failed\n";
#endif
			return false;
		}

		retcode = SQLConnect(_hDbc, (SQLWCHAR*)_database.c_str(),
			SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

		if ((retcode != SQL_SUCCESS) &&
			(retcode != SQL_SUCCESS_WITH_INFO))
		{
#ifdef _DEBUG
			std::cout << "SQLConnect Failed\n";
#endif
			return false;
		}
	}

	return true;
}
