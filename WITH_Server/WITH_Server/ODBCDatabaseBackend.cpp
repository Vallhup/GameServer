#include "pch.h"
#include "ODBCDatabaseBackend.h"

#include "DBConn.h"
#include "FrameworkLog.h"

namespace
{
	constexpr const char* kLogCategory = "Database";

	DBRequestId NextRequestId(std::atomic<DBRequestId>& next) noexcept
	{
		DBRequestId id = next.fetch_add(1, std::memory_order_acq_rel);
		if (id == InvalidDBRequestId)
			id = next.fetch_add(1, std::memory_order_acq_rel);
		return id;
	}
}

ODBCDatabaseBackend::ODBCDatabaseBackend(
	Config config,
	IExecutorIOSink& sink)
	: _config(std::move(config))
	, _sink(sink)
{
}

ODBCDatabaseBackend::~ODBCDatabaseBackend()
{
	Stop();
}

bool ODBCDatabaseBackend::Start() noexcept
{
	bool expected = false;
	if (!_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		return true;

	_acceptingCommands.store(true, std::memory_order_release);

	try
	{
		_worker = std::thread([this]() { WorkerLoop(); });
	}
	catch (...)
	{
		_acceptingCommands.store(false, std::memory_order_release);
		_running.store(false, std::memory_order_release);
		return false;
	}

	return true;
}

void ODBCDatabaseBackend::Stop() noexcept
{
	if (!_running.exchange(false, std::memory_order_acq_rel))
		return;

	_acceptingCommands.store(false, std::memory_order_release);
	_commandCv.notify_all();

	if (_worker.joinable())
		_worker.join();

	std::deque<DBCommandEnvelope> pending;
	{
		std::lock_guard lock{ _commandMutex };
		pending.swap(_commandQueue);
	}

	for (DBCommandEnvelope& command : pending)
	{
		PushRejectedCompletion(
			command,
			static_cast<uint32_t>(DBCommonErrorCode::Rejected),
			L"DB backend stopped before command execution.");
	}
}

DBRequestId ODBCDatabaseBackend::Submit(DBCommandEnvelope command) noexcept
{
	if (command.command == nullptr)
		return InvalidDBRequestId;

	if (!_acceptingCommands.load(std::memory_order_acquire))
	{
		PushRejectedCompletion(
			command,
			static_cast<uint32_t>(DBCommonErrorCode::Rejected),
			L"DB backend is not accepting commands.");
		return InvalidDBRequestId;
	}

	const DBRequestId requestId = NextRequestId(_nextRequestId);
	command.meta.requestId = requestId;

	{
		std::lock_guard lock{ _commandMutex };
		if (_config.commandQueueSoftLimit > 0 &&
			_commandQueue.size() >= _config.commandQueueSoftLimit)
		{
			PushRejectedCompletion(
				command,
				static_cast<uint32_t>(DBCommonErrorCode::Rejected),
				L"DB command queue soft limit exceeded.");
			return InvalidDBRequestId;
		}

		_commandQueue.push_back(std::move(command));
	}

	_commandCv.notify_one();
	return requestId;
}

bool ODBCDatabaseBackend::DrainCompletions(uint32_t) noexcept
{
	const uint32_t limit =
		_config.resultDrainLimitPerCall == 0
		? 256
		: _config.resultDrainLimitPerCall;

	bool drained = false;
	for (uint32_t i = 0; i < limit; ++i)
	{
		DBCompletion completion{};
		if (!_completionQueue.try_pop(completion))
			break;

		if (completion.completionTaskTypeId == InvalidDynamicTaskTypeId ||
			completion.scopeId == InvalidExecScopeId)
		{
			if (completion.payloadKey != InvalidDBPayloadKey)
				_resultStore.Drop(completion.payloadKey);

			FWLOG_WARN(kLogCategory,
				"DrainCompletions - dropping unroutable DB completion "
				"(requestId=%llu, taskType=%u, scopeId=%u)",
				static_cast<unsigned long long>(completion.requestId),
				completion.completionTaskTypeId,
				completion.scopeId);
			continue;
		}

		const DBCompletion routing = completion;
		const DBPayloadKey completionKey =
			_resultStore.PutCompletion(std::move(completion));

		DynamicTaskRequest request{};
		request.typeId = routing.completionTaskTypeId;
		request.scopeId = routing.scopeId;
		request.targetKind = DynamicTaskTargetKind::ExplicitScope;
		request.payloadKey = completionKey;
		request.requestFrameIndex = routing.requestFrameIndex;
		request.sessionId = routing.sessionId;

		_sink.SubmitDynamicTask(std::move(request));
		drained = true;
	}

	return drained;
}

void ODBCDatabaseBackend::PushDBCompletion(DBCompletion completion) noexcept
{
	_completionQueue.push(std::move(completion));
	_sink.WakeForExternalIO();
}

void ODBCDatabaseBackend::WorkerLoop() noexcept
{
	DBConn conn;

	// TODO(DB): Wire the actual MSSQL ODBC connection here.
	// The backend thread/queue/IIOBackend integration is implemented, but
	// SQLDriverConnectW/SQLConnectW should be enabled only after configuration,
	// credential handling, and integration-test setup are finalized.
	// Intended shape:
	//   if (!_config.connectionString.empty()) conn.ConnectConnStr(...);
	//   else conn.ConnectDSN(_config.dsn, _config.user, _config.password);

	while (_running.load(std::memory_order_acquire))
	{
		DBCommandEnvelope command{};
		if (!PopCommand(command))
			continue;

		if (command.command == nullptr)
			continue;

		try
		{
			DBCommandContext ctx
			{
				conn,
				command.meta,
				command.command->DebugTypeId(),
				_resultStore,
				*this
			};

			// TODO(DB): Real query objects are intentionally not implemented in
			// this pass. Commands added later should prepare static SQL through
			// ctx.Prepare(), bind parameters on DBStatement, and finish with
			// CompleteOk/Error.
			command.command->Execute(ctx);
		}
		catch (...)
		{
			PushRejectedCompletion(
				command,
				static_cast<uint32_t>(DBCommonErrorCode::Exception),
				L"DB command threw an exception.");
		}
	}

	// TODO(DB): Disconnect the actual ODBC connection here after connection
	// setup is enabled.
}

bool ODBCDatabaseBackend::PopCommand(DBCommandEnvelope& out) noexcept
{
	std::unique_lock lock{ _commandMutex };
	_commandCv.wait(lock, [this]()
	{
		return 
			!_commandQueue.empty() ||
			!_running.load(std::memory_order_acquire);
	});

	if (_commandQueue.empty())
		return false;

	out = std::move(_commandQueue.front());
	_commandQueue.pop_front();
	return true;
}

void ODBCDatabaseBackend::PushRejectedCompletion(
	DBCommandEnvelope& command,
	uint32_t errorCode,
	std::wstring message) noexcept
{
	DBCompletion completion{};
	completion.requestId = command.meta.requestId;
	completion.sessionId = command.meta.sessionId;
	completion.scopeId = command.meta.scopeId;
	completion.requestFrameIndex = command.meta.requestFrameIndex;
	completion.completionTaskTypeId = command.meta.completionTaskTypeId;
	completion.ok = false;
	completion.errorCode = errorCode;
	completion.message = std::move(message);

	if (command.command != nullptr)
		completion.debugCommandTypeId = command.command->DebugTypeId();

	PushDBCompletion(std::move(completion));
}
