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
		EntityType entityType)
	{
		Protocol::SC_ADD_PACKET add;
		add.set_netid(netId.GetRaw());
		add.set_typeid_(ToInt(entityType));
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
	, _inboundProcessor(InboundMessageProcessor::Dependencies{
		&_network,
		&_framework,
		&_startupWorldId,
		&_sessionBindings,
		&_pendingLoginSpawns
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
	_pendingLoginSpawns.clear();
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
	_pendingLoginSpawns.clear();
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
	std::vector<InboundMessage> remainingMessages;
	_inboundProcessor.Process(_inboundMessages, remainingMessages);
	_inboundMessages = std::move(remainingMessages);
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
		auto pendingIt = std::find_if(
			_pendingLoginSpawns.begin(),
			_pendingLoginSpawns.end(),
			[&](const InboundMessageProcessor::PendingLoginSpawn& pending)
			{
				return
					pending.worldId == spawnEvent.worldId &&
					pending.entity == spawnEvent.entity;
			});

		if (pendingIt == _pendingLoginSpawns.end())
		{
			continue;
		}

		if (!spawnEvent.netId.IsValid())
		{
			continue;
		}

		WorldInstance* world = _framework.FindWorld(spawnEvent.worldId);
		if (world == nullptr)
		{
			continue;
		}

		EntityType entityType = EntityType::Character;
		const ECSView view = world->GetRuntime().MakeView();
		if (const SpawnTypeComp* spawnType =
			view.GetComponent<SpawnTypeComp>(spawnEvent.entity))
		{
			entityType = spawnType->entityType;
		}

		const SessionId sessionId = pendingIt->sessionId;
		(void)_sessionBindings.Bind(sessionId, spawnEvent.netId, spawnEvent.worldId);
		(void)_network.RequestEnterInGame(sessionId, spawnEvent.netId);
		(void)StageLoginResponse(_network, sessionId, spawnEvent.netId);
		(void)StageSpawnAddPacket(_network, sessionId, spawnEvent.netId, entityType);

		_pendingLoginSpawns.erase(pendingIt);
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
