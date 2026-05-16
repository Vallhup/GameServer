#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include <concurrent_queue.h>

#include "DBData.h"
#include "IExecutorIOSink.h"
#include "IIOBackend.h"

class ODBCDatabaseBackend final
	: public IIOBackend
	, private IDBCompletionSink
{
public:
	struct Config
	{
		bool enabled{ false };
		std::wstring connectionString;
		std::wstring dsn;
		std::wstring user;
		std::wstring password;
		uint32_t commandQueueSoftLimit{ 4096 };
		uint32_t resultDrainLimitPerCall{ 256 };
	};

	ODBCDatabaseBackend(Config config, IExecutorIOSink& sink);
	~ODBCDatabaseBackend();

	ODBCDatabaseBackend(const ODBCDatabaseBackend&) = delete;
	ODBCDatabaseBackend& operator=(const ODBCDatabaseBackend&) = delete;

	bool Start() noexcept;
	void Stop() noexcept;

	[[nodiscard]]
	DBRequestId Submit(DBCommandEnvelope command) noexcept;

	[[nodiscard]]
	bool DrainCompletions(uint32_t workerIdx) noexcept override;

	[[nodiscard]]
	const char* DebugName() const noexcept override { return "DB"; }

	DBResultStore& ResultStore() noexcept { return _resultStore; }
	const DBResultStore& ResultStore() const noexcept { return _resultStore; }

private:
	void PushDBCompletion(DBCompletion completion) noexcept override;

	void WorkerLoop() noexcept;
	bool PopCommand(DBCommandEnvelope& out) noexcept;
	void PushRejectedCompletion(
		DBCommandEnvelope& command,
		uint32_t errorCode,
		std::wstring message) noexcept;

private:
	Config _config;
	IExecutorIOSink& _sink;

	std::atomic<bool> _running{ false };
	std::atomic<bool> _acceptingCommands{ false };
	std::atomic<DBRequestId> _nextRequestId{ 1 };

	std::thread _worker;

	std::mutex _commandMutex;
	std::condition_variable _commandCv;
	std::deque<DBCommandEnvelope> _commandQueue;

	concurrency::concurrent_queue<DBCompletion> _completionQueue;
	DBResultStore _resultStore;
};
