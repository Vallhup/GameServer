#include "pch.h"
#include "NetworkRuntime.h"

#include <algorithm>
#include <shared_mutex>
#include <utility>

#include "Connection.h"
#include "GameSendBufferManager.h"
#include "SendBuffer.h"
#include "SessionManager.h"

struct NetworkRuntime::Impl
{
	explicit Impl(const Config& cfg)
		: config(cfg)
	{
	}

	void RefreshLiveSessionIds()
	{
		std::vector<SessionId> snapshot;
		sessionManager.FillSessionIds(snapshot);

		std::unique_lock lock{ liveSessionIdsMutex };
		liveSessionIds = std::move(snapshot);
	}

	std::vector<SessionId> CopyLiveSessionIds() const
	{
		std::shared_lock lock{ liveSessionIdsMutex };
		return liveSessionIds;
	}

	Config config;

	std::atomic<bool> initialized{ false };
	std::atomic<bool> running{ false };
	std::atomic<uint32_t> sessionCount{ 0 };

	std::unique_ptr<GameServerNetworkListener> listener;
	std::unique_ptr<GameServerNetworkService> service;
	std::unique_ptr<asio::strand<asio::any_io_executor>> controlStrand;

	SessionManager sessionManager;
	GameSendBufferManager sendBufferManager;
	concurrency::concurrent_queue<InboundMessage> inboundQueue;

	mutable std::shared_mutex liveSessionIdsMutex;
	std::vector<SessionId> liveSessionIds;
};

NetworkRuntime::NetworkRuntime(Config config)
	: _config(config)
{
}

NetworkRuntime::~NetworkRuntime()
{
	Shutdown();
}

bool NetworkRuntime::Initialize()
{
	if (IsInitialized())
	{
		return true;
	}

	_impl = std::make_unique<Impl>(_config);
	_impl->listener = std::make_unique<GameServerNetworkListener>(*this);
	_impl->service = std::make_unique<GameServerNetworkService>(
		_config.workerThreadCount,
		_config.listenPort,
		_config.maxPacketSize,
		*_impl->listener,
		*this);
	_impl->controlStrand =
		std::make_unique<asio::strand<asio::any_io_executor>>(
			_impl->service->GetExecutor());

	_impl->initialized.store(true);
	return true;
}

void NetworkRuntime::Start()
{
	if (!_impl || _impl->running.load())
	{
		return;
	}

	_impl->service->Start();
	_impl->running.store(true);
}

void NetworkRuntime::Stop() noexcept
{
	if (!_impl || !_impl->running.exchange(false))
	{
		return;
	}

	_impl->service->Stop();
}

void NetworkRuntime::Shutdown() noexcept
{
	Stop();

	if (!_impl)
	{
		return;
	}

	_impl->sendBufferManager.Reset();
	_impl->initialized.store(false);
	_impl.reset();
}

bool NetworkRuntime::IsInitialized() const noexcept
{
	return _impl != nullptr && _impl->initialized.load();
}

bool NetworkRuntime::IsRunning() const noexcept
{
	return _impl != nullptr && _impl->running.load();
}

void NetworkRuntime::DrainInboundMessages(std::vector<InboundMessage>& outMessages)
{
	outMessages.clear();

	if (!_impl)
	{
		return;
	}

	InboundMessage message{};
	while (_impl->inboundQueue.try_pop(message))
	{
		outMessages.push_back(std::move(message));
	}
}

void NetworkRuntime::BeginSendStage() noexcept
{
	if (!_impl)
	{
		return;
	}

	_impl->sendBufferManager.BeginSendStage();
}

bool NetworkRuntime::StageUnicast(
	SessionId sessionId,
	std::span<const uint8_t> payload)
{
	if (!_impl)
	{
		return false;
	}

	return _impl->sendBufferManager.StageUnicast(sessionId, payload);
}

bool NetworkRuntime::StageMulticast(
	std::span<const SessionId> sessionIds,
	std::span<const uint8_t> payload)
{
	if (!_impl)
	{
		return false;
	}

	return _impl->sendBufferManager.StageMulticast(sessionIds, payload);
}

bool NetworkRuntime::StageBroadcastAll(std::span<const uint8_t> payload)
{
	if (!_impl)
	{
		return false;
	}

	const std::vector<SessionId> sessionIds = _impl->CopyLiveSessionIds();
	return _impl->sendBufferManager.StageMulticast(sessionIds, payload);
}

bool NetworkRuntime::StageBroadcastExcept(
	SessionId exceptSessionId,
	std::span<const uint8_t> payload)
{
	if (!_impl)
	{
		return false;
	}

	const std::vector<SessionId> liveSessionIds = _impl->CopyLiveSessionIds();
	std::vector<SessionId> targets;
	targets.reserve(liveSessionIds.size());

	for (const SessionId sessionId : liveSessionIds)
	{
		if (sessionId == exceptSessionId)
		{
			continue;
		}

		targets.push_back(sessionId);
	}

	return _impl->sendBufferManager.StageMulticast(targets, payload);
}

bool NetworkRuntime::StageBroadcastExceptMany(
	std::span<const SessionId> exceptSessionIds,
	std::span<const uint8_t> payload)
{
	if (!_impl)
	{
		return false;
	}

	const std::vector<SessionId> liveSessionIds = _impl->CopyLiveSessionIds();
	std::vector<SessionId> targets;
	targets.reserve(liveSessionIds.size());

	for (const SessionId sessionId : liveSessionIds)
	{
		const bool isExcluded =
			std::find(
				exceptSessionIds.begin(),
				exceptSessionIds.end(),
				sessionId) != exceptSessionIds.end();

		if (!isExcluded)
		{
			targets.push_back(sessionId);
		}
	}

	return _impl->sendBufferManager.StageMulticast(targets, payload);
}

void NetworkRuntime::FlushSendStage()
{
	if (!_impl || !_impl->controlStrand)
	{
		return;
	}

	auto batch = std::make_shared<GameSendBufferManager::FlushBatch>(
		_impl->sendBufferManager.FlushSendStage());

	asio::post(*_impl->controlStrand, [this, batch]()
	{
		if (!_impl)
		{
			for (auto& staged : batch->stagedSends)
			{
				if (staged.buffer != nullptr)
				{
					SendBufferPool::Get().Release(staged.buffer);
					staged.buffer = nullptr;
				}
			}
			return;
		}

		for (auto& staged : batch->stagedSends)
		{
			if (staged.buffer == nullptr)
			{
				continue;
			}

			if (staged.targetSessionIds.empty())
			{
				SendBufferPool::Get().Release(staged.buffer);
				staged.buffer = nullptr;
				continue;
			}

			if (staged.targetSessionIds.size() == 1)
			{
				Session* const session =
					_impl->sessionManager.FindSession(staged.targetSessionIds.front());

				if (session == nullptr || !session->Send(staged.buffer))
				{
					SendBufferPool::Get().Release(staged.buffer);
				}

				staged.buffer = nullptr;
				continue;
			}

			for (const SessionId sessionId : staged.targetSessionIds)
			{
				Session* const session = _impl->sessionManager.FindSession(sessionId);
				if (session == nullptr)
				{
					continue;
				}

				SendBuffer* const cloned = staged.buffer->Clone();
				if (cloned == nullptr)
				{
					continue;
				}

				if (!session->Send(cloned))
				{
					SendBufferPool::Get().Release(cloned);
				}
			}

			SendBufferPool::Get().Release(staged.buffer);
			staged.buffer = nullptr;
		}
	});
}

bool NetworkRuntime::RequestCompleteLogin(SessionId sessionId)
{
	if (!_impl || !_impl->controlStrand)
	{
		return false;
	}

	asio::post(*_impl->controlStrand, [this, sessionId]()
	{
		if (!_impl)
		{
			return;
		}

		Session* const session = _impl->sessionManager.FindSession(sessionId);
		if (session != nullptr)
		{
			(void)session->CompleteLogin();
		}
	});

	return true;
}

bool NetworkRuntime::RequestEnterInGame(SessionId sessionId, NetId playerNetId)
{
	if (!_impl || !_impl->controlStrand)
	{
		return false;
	}

	asio::post(*_impl->controlStrand, [this, sessionId, playerNetId]()
	{
		if (!_impl)
		{
			return;
		}

		Session* const session = _impl->sessionManager.FindSession(sessionId);
		if (session != nullptr)
		{
			(void)session->EnterInGame(playerNetId);
		}
	});

	return true;
}

bool NetworkRuntime::RequestLeaveGame(SessionId sessionId)
{
	if (!_impl || !_impl->controlStrand)
	{
		return false;
	}

	asio::post(*_impl->controlStrand, [this, sessionId]()
	{
		if (!_impl)
		{
			return;
		}

		Session* const session = _impl->sessionManager.FindSession(sessionId);
		if (session != nullptr)
		{
			(void)session->LeaveGame();
		}
	});

	return true;
}

bool NetworkRuntime::RequestClose(SessionId sessionId, SessionCloseReason reason)
{
	if (!_impl || !_impl->controlStrand)
	{
		return false;
	}

	asio::post(*_impl->controlStrand, [this, sessionId, reason]()
	{
		if (!_impl)
		{
			return;
		}

		(void)_impl->sessionManager.BeginClose(sessionId, reason);
	});

	return true;
}

uint32 NetworkRuntime::GetApproxSessionCount() const noexcept
{
	if (!_impl)
	{
		return 0;
	}

	return _impl->sessionCount.load();
}

void NetworkRuntime::OnAcceptedConnection(
	const std::shared_ptr<Connection>& connection)
{
	if (!_impl || !_impl->controlStrand || connection == nullptr)
	{
		return;
	}

	asio::post(*_impl->controlStrand, [this, connection]()
	{
		if (!_impl)
		{
			return;
		}

		if (_impl->config.maxSessions > 0 &&
			_impl->sessionManager.GetSessionCount() >= _impl->config.maxSessions)
		{
			connection->Stop();
			return;
		}

		Session* const session = _impl->sessionManager.CreateSession(connection);
		if (session == nullptr)
		{
			connection->Stop();
			return;
		}

		_impl->RefreshLiveSessionIds();
		_impl->sessionCount.store(_impl->sessionManager.GetSessionCount());
		connection->Start();
	});
}

void NetworkRuntime::OnInboundMessage(InboundMessage&& message)
{
	if (!_impl)
	{
		return;
	}

	const InboundMessageKind kind = message.kind;
	const SessionId sessionId = message.sessionId;
	_impl->inboundQueue.push(std::move(message));

	if (kind != InboundMessageKind::Disconnected || !_impl->controlStrand)
	{
		return;
	}

	asio::post(*_impl->controlStrand, [this, sessionId]()
	{
		if (!_impl)
		{
			return;
		}

		(void)_impl->sessionManager.MarkClosed(
			sessionId,
			SessionCloseReason::RemoteClosed);

		(void)_impl->sessionManager.RemoveSession(sessionId);

		_impl->RefreshLiveSessionIds();
		_impl->sessionCount.store(_impl->sessionManager.GetSessionCount());
	});
}
