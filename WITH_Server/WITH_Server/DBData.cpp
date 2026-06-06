#include "pch.h"
#include "DBData.h"

DBResultStore::~DBResultStore()
{
	Clear();
}

DBPayloadKey DBResultStore::PutCompletion(DBCompletion completion)
{
	std::lock_guard lock{ _mutex };
	const DBPayloadKey key = AllocateKeyLocked();
	_completions.emplace(key, std::move(completion));
	return key;
}

bool DBResultStore::TakeCompletion(DBPayloadKey key, DBCompletion& out)
{
	std::lock_guard lock{ _mutex };
	const auto it = _completions.find(key);
	if (it == _completions.end())
		return false;

	out = std::move(it->second);
	_completions.erase(it);
	return true;
}

bool DBResultStore::Drop(DBPayloadKey key) noexcept
{
	std::lock_guard lock{ _mutex };
	bool dropped = false;

	const auto completionIt = _completions.find(key);
	if (completionIt != _completions.end())
	{
		_completions.erase(completionIt);
		dropped = true;
	}

	const auto payloadIt = _payloads.find(key);
	if (payloadIt != _payloads.end())
	{
		DestroyPayload(payloadIt->second);
		_payloads.erase(payloadIt);
		dropped = true;
	}

	return dropped;
}

void DBResultStore::ClearExpired(uint64_t currentFrameIndex) noexcept
{
	(void)currentFrameIndex;
	// TODO(DB): Add frame-age or wall-clock retention policy once DB runtime
	// ownership is wired into the server frame lifecycle.
}

void DBResultStore::Clear() noexcept
{
	std::lock_guard lock{ _mutex };

	for (auto& [_, entry] : _payloads)
		DestroyPayload(entry);

	_payloads.clear();
	_completions.clear();
}

DBPayloadKey DBResultStore::AllocateKeyLocked() noexcept
{
	if (_nextKey == InvalidDBPayloadKey)
		++_nextKey;

	return _nextKey++;
}

void DBResultStore::DestroyPayload(PayloadEntry& entry) noexcept
{
	if (entry.object != nullptr && entry.destroy != nullptr)
		entry.destroy(entry.object);

	entry.object = nullptr;
	entry.destroy = nullptr;
	entry.moveOut = nullptr;
	entry.typeId = InvalidDBPayloadTypeId;
}

DBCommandContext::DBCommandContext(
	DBConn& conn,
	const DBRequestMeta& meta,
	DBCommandTypeId debugCommandTypeId,
	DBResultStore& resultStore,
	IDBCompletionSink& completionSink) noexcept
	: _conn(conn)
	, _meta(meta)
	, _debugCommandTypeId(debugCommandTypeId)
	, _resultStore(resultStore)
	, _completionSink(completionSink)
{
}

bool DBCommandContext::Prepare(DBQueryText sql, DBStatement& outStmt)
{
	return _conn.Prepare(sql, outStmt);
}

const DBErrorInfo& DBCommandContext::LastDBErrorInfo() const noexcept
{
	return _conn.LastErrorInfo();
}

std::wstring DBCommandContext::LastDBError() const
{
	return _conn.LastError();
}

void DBCommandContext::CompleteError(
	uint32_t errorCode,
	std::wstring message) noexcept
{
	DBCompletion completion = MakeBaseCompletion();
	completion.ok = false;
	completion.errorCode = errorCode;
	completion.message = std::move(message);

	_completionSink.PushDBCompletion(std::move(completion));
}

DBCompletion DBCommandContext::MakeBaseCompletion() const noexcept
{
	DBCompletion completion{};
	completion.requestId = _meta.requestId;
	completion.debugCommandTypeId = _debugCommandTypeId;
	completion.sessionId = _meta.sessionId;
	completion.clientRequestId = _meta.clientRequestId;
	completion.scopeId = _meta.scopeId;
	completion.requestFrameIndex = _meta.requestFrameIndex;
	completion.completionTaskTypeId = _meta.completionTaskTypeId;
	return completion;
}
