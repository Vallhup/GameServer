#include "pch.h"
#include "NetworkRuntime.h"

NetworkRuntime::NetworkRuntime(Config config)
	: _config(config)
{
}

NetworkRuntime::~NetworkRuntime()
{
	Shutdown();
}

void NetworkRuntime::OnConnectionAccepted(IocpConnection* connection) noexcept
{
	_sessionManager.CreateSession(connection);
}

bool NetworkRuntime::Initialize()
{
	if (_initialized.load())
		return true;

	if (_sink == nullptr)
		return false;

	IocpNetworkBackend::Config backendCfg{
		.listenPort    = _config.listenPort,
		.maxSessions   = _config.maxSessions,
		.maxPacketSize = _config.maxPacketSize
	};

	_backend = std::make_unique<IocpNetworkBackend>(
		backendCfg, *_sink, *this, _dispatchTable);

	_initialized.store(true);
	return true;
}

void NetworkRuntime::Start()
{
	if (!_initialized.load() || _running.load())
		return;

	if (_backend->Start())
		_running.store(true);
}

void NetworkRuntime::Stop() noexcept
{
	if (!_running.exchange(false))
		return;

	if (_backend)
		_backend->Stop();
}

void NetworkRuntime::Shutdown() noexcept
{
	Stop();

	if (!_initialized.exchange(false))
		return;

	_backend.reset();
}

bool NetworkRuntime::IsInitialized() const noexcept
{
	return _initialized.load();
}

bool NetworkRuntime::IsRunning() const noexcept
{
	return _running.load();
}

void NetworkRuntime::RegisterPacketHandler(uint16_t packetType, DynamicTaskTypeId typeId)
{
	_dispatchTable.Register(packetType, typeId);
}

void NetworkRuntime::BeginSendStage() noexcept
{
	// Chain-flushing: sends are posted immediately by RegisterSend.
	// No staging accumulation needed.
}

bool NetworkRuntime::StageUnicast(SessionId sessionId, std::span<const uint8_t> payload)
{
	if (!_backend) return false;
	return _backend->Send(sessionId, payload);
}

bool NetworkRuntime::StageMulticast(
	std::span<const SessionId> sessionIds,
	std::span<const uint8_t> payload)
{
	if (!_backend) return false;

	bool allOk = true;
	for (const SessionId id : sessionIds)
		allOk &= _backend->Send(id, payload);
	return allOk;
}

bool NetworkRuntime::StageBroadcastAll(std::span<const uint8_t> payload)
{
	if (!_backend) return false;

	std::vector<SessionId> ids;
	_sessionManager.FillSessionIds(ids);

	bool allOk = true;
	for (const SessionId id : ids)
		allOk &= _backend->Send(id, payload);
	return allOk;
}

bool NetworkRuntime::StageBroadcastExcept(
	SessionId exceptSessionId,
	std::span<const uint8_t> payload)
{
	if (!_backend) return false;

	std::vector<SessionId> ids;
	_sessionManager.FillSessionIds(ids);

	bool allOk = true;
	for (const SessionId id : ids)
	{
		if (id == exceptSessionId) continue;
		allOk &= _backend->Send(id, payload);
	}
	return allOk;
}

bool NetworkRuntime::StageBroadcastExceptMany(
	std::span<const SessionId> exceptSessionIds,
	std::span<const uint8_t> payload)
{
	if (!_backend) return false;

	std::vector<SessionId> ids;
	_sessionManager.FillSessionIds(ids);

	bool allOk = true;
	for (const SessionId id : ids)
	{
		const bool excluded = std::find(
			exceptSessionIds.begin(), exceptSessionIds.end(), id)
			!= exceptSessionIds.end();
		if (excluded) continue;
		allOk &= _backend->Send(id, payload);
	}
	return allOk;
}

void NetworkRuntime::FlushSendStage()
{
	if (_backend)
		_backend->FlushSend();
}

bool NetworkRuntime::RequestClose(SessionId sessionId, SessionCloseReason reason)
{
	if (!_backend) return false;
	_backend->Disconnect(sessionId, reason);
	return true;
}

uint32_t NetworkRuntime::GetApproxSessionCount() const noexcept
{
	return _sessionManager.GetSessionCount();
}

INetworkBackend& NetworkRuntime::GetNetworkBackend() noexcept
{
	return *_backend;
}
