#include "pch.h"
#include "ServerApp.h"

#include <algorithm>
#include <thread>

#include "PacketFactory.h"
#include "Protocol.pb.h"
#include "RepComponent.h"
#include "WorldInstance.h"

namespace
{
	bool StageLoginResponse(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId playerNetId)
	{
		Protocol::SC_LOGIN_PACKET loginAck;
		loginAck.set_netid(playerNetId.GetRaw());

		SendBuffer* const buffer =
			PacketFactory::Serialize(PacketType::SC_LOGIN, loginAck);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged = network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}

	bool StageSpawnAddPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		CharacterId characterId)
	{
		Protocol::SC_ADD_PACKET add;
		add.set_netid(netId.GetRaw());
		add.set_typeid_(static_cast<int>(characterId));
		add.set_x(0.0f);
		add.set_y(0.0f);
		add.set_z(0.0f);
		add.set_yaw(0.0f);

		SendBuffer* const buffer =
			PacketFactory::Serialize(PacketType::SC_ADD, add);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged = network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}
}

ServerApp::ServerApp(Config config)
	: _config(config)
	, _framework(FrameworkRuntime::Config{
		.executorWorkerCount = _config.executorWorkerCount
	})
	, _network(NetworkRuntime::Config{
		.workerThreadCount = _config.networkThreadCount,
		.listenPort = _config.listenPort,
		.maxSessions = _config.maxSessions
	})
	, _playerEntryService(PlayerEntryService::Dependencies{
		&_framework,
		&_startupWorldId
	})
	, _inboundProcessor(InboundMessageProcessor::Dependencies{
		&_network,
		&_playerEntryService,
		&_sessionBindings,
	})
{
	if (_config.logicTickHz == 0)
	{
		_config.logicTickHz = 30;
	}
}

bool ServerApp::Initialize()
{
	bool expected = false;
	if (!_initialized.compare_exchange_strong(expected, true))
	{
		return true;
	}

	_stopRequested.store(false);
	_tickCount = 0;
	_frameIndex = 1;
	_nowSec = 0.0;
	_startupWorldId = WorldId::Invalid();
	_sessionBindings.Clear();
	_playerEntryService.Clear();
	_lastTickTime = {};

	if (!InitializeFrameworkRuntime() ||
		!InitializeNetworkRuntime() ||
		!InitializeGameplayContent())
	{
		_network.Shutdown();
		_framework.Shutdown();
		_initialized.store(false);
		return false;
	}

	return true;
}

void ServerApp::Run()
{
	if (_running.load())
	{
		return;
	}

	if (!IsInitialized() && !Initialize())
	{
		return;
	}

	_stopRequested.store(false);
	_running.store(true);
	RunLogicLoop();
	_running.store(false);
}

void ServerApp::Stop() noexcept
{
	_stopRequested.store(true);
}

void ServerApp::Shutdown() noexcept
{
	Stop();
	_running.store(false);
	_network.Shutdown();
	_framework.Shutdown();
	_initialized.store(false);
	_tickCount = 0;
	_frameIndex = 1;
	_nowSec = 0.0;
	_startupWorldId = WorldId::Invalid();
	_sessionBindings.Clear();
	_playerEntryService.Clear();
	_lastTickTime = {};
}

bool ServerApp::InitializeFrameworkRuntime()
{
	FrameworkRuntime::BootstrapParams bootstrapParams{};
	bootstrapParams.worldFactory = &_bootstrapFactory;
	bootstrapParams.definitionProvider = &_bootstrapDefinitions;

	if (!_framework.Initialize(bootstrapParams))
	{
		std::cout << "[ServerApp] Framework bootstrap failed.\n";
		return false;
	}

	return InitializeStartupWorld();
}

bool ServerApp::InitializeNetworkRuntime()
{
	if (!_network.Initialize())
	{
		std::cout << "[ServerApp] Network runtime initialize failed.\n";
		return false;
	}

	_network.Start();
	return true;
}

bool ServerApp::InitializeGameplayContent()
{
	// Future work:
	// - load animations / maps / gameplay data
	// - register world factories and gameplay bindings
	return true;
}

bool ServerApp::InitializeStartupWorld()
{
	// Square is currently modeled as a pre-created persistent hub world.
	constexpr WorldDefId startupWorldDefId = WorldDefId::Square;
	constexpr uint64_t startupInstanceKey = 0;

	_startupWorldId = _framework.RegisterPreCreatedWorld(
		startupWorldDefId,
		startupInstanceKey);

	if (!_startupWorldId.IsValid())
	{
		std::cout << "[ServerApp] Startup world creation failed."
			<< " defId=" << static_cast<int>(startupWorldDefId)
			<< " instanceKey=" << startupInstanceKey << "\n";
		return false;
	}

	if (!_framework.InitializeWorld(_startupWorldId))
	{
		std::cout << "[ServerApp] Startup world initialization failed."
			<< " worldId=" << _startupWorldId.GetRaw() << "\n";
		return false;
	}

	return true;
}

void ServerApp::RunLogicLoop()
{
	using clock = std::chrono::steady_clock;

	const double fixedDt = 1.0 / static_cast<double>(_config.logicTickHz);
	auto previous = clock::now();
	double accumulator = 0.0;

	while (!_stopRequested.load())
	{
		const auto now = clock::now();
		accumulator += std::chrono::duration<double>(now - previous).count();
		previous = now;

		while (accumulator >= fixedDt && !_stopRequested.load())
		{
			TickOnce(fixedDt);
			accumulator -= fixedDt;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void ServerApp::TickOnce(double dtSec)
{
	_lastTickTime = std::chrono::steady_clock::now();

	DrainInboundCommands();
	_network.BeginSendStage();
	ProcessInboundMessages();
	RunWorldFrames(dtSec);
	BuildReplication();
	FlushOutbound();

	++_tickCount;
}

void ServerApp::DrainInboundCommands()
{
	_network.DrainInboundMessages(_inboundMessages);
}

void ServerApp::ProcessInboundMessages()
{
	_inboundProcessor.Process(_inboundMessages, _inboundMessages);
}

void ServerApp::RunWorldFrames(double dtSec)
{
	_nowSec += dtSec;

	if (!_framework.TickServices(_nowSec, dtSec))
	{
		std::cout << "[ServerApp] Framework service tick failed.\n";
		Stop();
		return;
	}

	FrameworkRuntime::FrameResult frameResult{};
	FrameworkRuntime::FrameParams frameParams{};
	frameParams.frameIndex = _frameIndex;
	frameParams.nowSec = _nowSec;
	frameParams.dtSec = dtSec;

	const bool frameOk =
		_framework.RunFrame(frameParams, frameResult);

	if (!frameOk)
	{
		std::cout << "[ServerApp] Frame execution failed."
			<< " frameIndex=" << _frameIndex
			<< " reason=" << static_cast<int>(frameResult.failureReason)
			<< " selectedWorlds=" << frameResult.selectedWorldCount
			<< "\n";
		Stop();
		return;
	}

	FinalizeFrameEvents(frameResult);
	++_frameIndex;
}

void ServerApp::FinalizeFrameEvents(const FrameworkRuntime::FrameResult& frameResult)
{
	for (const auto& spawnEvent : frameResult.events.spawns)
	{
		if (!spawnEvent.netId.IsValid())
		{
			continue;
		}

		if (_playerEntryService.FindPendingSpawn(
			spawnEvent.worldId,
			spawnEvent.entity) == nullptr)
		{
			continue;
		}

		PendingCharacterSpawn pendingSpawn{};
		if (!_playerEntryService.TryConsumeSpawnConfirmed(
			spawnEvent.worldId,
			spawnEvent.entity,
			pendingSpawn))
		{
			continue;
		}

		const SessionId sessionId = pendingSpawn.sessionId;
		(void)_sessionBindings.Bind(sessionId, spawnEvent.netId, spawnEvent.worldId);
		(void)_network.RequestEnterInGame(sessionId, spawnEvent.netId);
		(void)StageLoginResponse(_network, sessionId, spawnEvent.netId);
		(void)StageSpawnAddPacket(
			_network,
			sessionId,
			spawnEvent.netId,
			pendingSpawn.characterId);
	}
}

void ServerApp::BuildReplication()
{
	// Future work:
	// - build staged unicast/multicast packets via _network.Stage*
}

void ServerApp::FlushOutbound()
{
	_network.FlushSendStage();
}
