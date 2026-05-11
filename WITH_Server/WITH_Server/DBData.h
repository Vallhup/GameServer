#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "DBConn.h"
#include "DynamicTaskTypes.h"
#include "ExecutionCoreTypes.h"

using DBRequestId = uint64_t;
using DBPayloadKey = uint64_t;
using DBPayloadTypeId = uint32_t;
using DBCommandTypeId = uint32_t;

constexpr DBRequestId InvalidDBRequestId = 0;
constexpr DBPayloadKey InvalidDBPayloadKey = 0;
constexpr DBPayloadTypeId InvalidDBPayloadTypeId = 0;
constexpr DBCommandTypeId InvalidDBCommandTypeId = 0;

enum class DBCommonErrorCode : uint32_t
{
	None = 0,
	Rejected = 1,
	ConnectFail = 2,
	QueryFail = 3,
	Timeout = 4,
	Exception = 5,
};

struct DBRequestMeta
{
	DBRequestId requestId{ InvalidDBRequestId };

	uint32_t sessionId{ 0 };
	ExecScopeId scopeId{ InvalidExecScopeId };
	uint64_t requestFrameIndex{ 0 };

	DynamicTaskTypeId completionTaskTypeId{ InvalidDynamicTaskTypeId };
};

struct DBCompletion
{
	DBRequestId requestId{ InvalidDBRequestId };
	DBCommandTypeId debugCommandTypeId{ InvalidDBCommandTypeId };

	uint32_t sessionId{ 0 };
	ExecScopeId scopeId{ InvalidExecScopeId };
	uint64_t requestFrameIndex{ 0 };

	DynamicTaskTypeId completionTaskTypeId{ InvalidDynamicTaskTypeId };

	bool ok{ false };
	uint32_t errorCode{ 0 };
	std::wstring message;

	DBPayloadKey payloadKey{ InvalidDBPayloadKey };
	DBPayloadTypeId payloadTypeId{ InvalidDBPayloadTypeId };
};

class DBResultStore
{
public:
	DBResultStore() = default;
	~DBResultStore();

	DBResultStore(const DBResultStore&)			   = delete;
	DBResultStore& operator=(const DBResultStore&) = delete;

	[[nodiscard]]
	DBPayloadKey PutCompletion(DBCompletion completion);

	[[nodiscard]]
	bool TakeCompletion(DBPayloadKey key, DBCompletion& out);

	template<class T>
	[[nodiscard]]
	DBPayloadKey PutPayload(T&& payload)
	{
		using PayloadT = std::decay_t<T>;
		static_assert(std::is_move_constructible_v<PayloadT>);
		static_assert(requires { PayloadT::PayloadTypeId; });

		auto* object = new PayloadT(std::forward<T>(payload));

		PayloadEntry entry{};
		entry.typeId = PayloadT::PayloadTypeId;
		entry.object = object;
		entry.destroy = [](void* ptr) noexcept
		{
			delete static_cast<PayloadT*>(ptr);
		};
		entry.moveOut = [](void* src, void* dst) noexcept
		{
			*static_cast<PayloadT*>(dst) =
				std::move(*static_cast<PayloadT*>(src));
		};

		std::lock_guard lock{ _mutex };
		const DBPayloadKey key = AllocateKeyLocked();
		_payloads.emplace(key, entry);
		return key;
	}

	template<class T>
	[[nodiscard]]
	bool TakePayload(DBPayloadKey key, T& out)
	{
		using PayloadT = std::decay_t<T>;
		static_assert(requires { PayloadT::PayloadTypeId; });

		PayloadEntry entry{};
		{
			std::lock_guard lock{ _mutex };
			const auto it = _payloads.find(key);
			if (it == _payloads.end() ||
				it->second.typeId != PayloadT::PayloadTypeId)
			{
				return false;
			}

			entry = it->second;
			_payloads.erase(it);
		}

		entry.moveOut(entry.object, &out);
		entry.destroy(entry.object);
		return true;
	}

	bool Drop(DBPayloadKey key) noexcept;
	void ClearExpired(uint64_t currentFrameIndex) noexcept;
	void Clear() noexcept;

private:
	struct PayloadEntry
	{
		DBPayloadTypeId typeId{ InvalidDBPayloadTypeId };
		void* object{ nullptr };

		void (*destroy)(void*) noexcept { nullptr };
		void (*moveOut)(void* src, void* dst) noexcept { nullptr };

		uint64_t createdFrameIndex{ 0 };
	};

	[[nodiscard]]
	DBPayloadKey AllocateKeyLocked() noexcept;

	void DestroyPayload(PayloadEntry& entry) noexcept;

private:
	std::mutex _mutex;
	DBPayloadKey _nextKey{ 1 };
	std::unordered_map<DBPayloadKey, DBCompletion> _completions;
	std::unordered_map<DBPayloadKey, PayloadEntry> _payloads;
};

class IDBCompletionSink
{
public:
	virtual ~IDBCompletionSink() = default;
	virtual void PushDBCompletion(DBCompletion completion) noexcept = 0;
};

class DBCommandContext
{
public:
	DBCommandContext(
		DBConn& conn,
		const DBRequestMeta& meta,
		DBCommandTypeId debugCommandTypeId,
		DBResultStore& resultStore,
		IDBCompletionSink& completionSink) noexcept;

	const DBRequestMeta& Meta() const noexcept { return _meta; }

	[[nodiscard]]
	bool Prepare(DBQueryText sql, DBStatement& outStmt);

	[[nodiscard]]
	const DBErrorInfo& LastDBErrorInfo() const noexcept;

	[[nodiscard]]
	std::wstring LastDBError() const;

	template<class T>
	void CompleteOk(T&& payload) noexcept
	{
		using PayloadT = std::decay_t<T>;
		static_assert(requires { PayloadT::PayloadTypeId; });

		const DBPayloadKey payloadKey =
			_resultStore.PutPayload(std::forward<T>(payload));

		DBCompletion completion = MakeBaseCompletion();
		completion.ok			 = true;
		completion.payloadKey	 = payloadKey;
		completion.payloadTypeId = PayloadT::PayloadTypeId;

		_completionSink.PushDBCompletion(std::move(completion));
	}

	void CompleteError(
		uint32_t errorCode,
		std::wstring message = {}) noexcept;

private:
	[[nodiscard]]
	DBCompletion MakeBaseCompletion() const noexcept;

private:
	DBCommandTypeId		 _debugCommandTypeId{ InvalidDBCommandTypeId };

	DBConn&				 _conn;
	DBResultStore&		 _resultStore;
	IDBCompletionSink&	 _completionSink;
	const DBRequestMeta& _meta;
};

struct IDBCommand
{
	virtual ~IDBCommand() = default;

	virtual DBCommandTypeId DebugTypeId() const noexcept = 0;
	virtual const char*		DebugName()   const noexcept = 0;

	virtual void Execute(DBCommandContext& ctx) noexcept = 0;
};

struct DBCommandEnvelope
{
	DBRequestMeta meta;
	std::unique_ptr<IDBCommand> command;
};
