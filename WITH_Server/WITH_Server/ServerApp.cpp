#include "pch.h"
#include "ServerApp.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <thread>
#include <stdlib.h>

#include "ServerDirtyReplicationService.h"
#include "ServerFrameEventDispatcher.h"
#include "ServerPathResolver.h"
#include "ServerWorldTransferCommitter.h"

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
	, _transferBinding(_framework, _sessionBindings)
	, _playerEntryService(PlayerEntryService::Dependencies{
		&_framework,
		&_startupWorldId
	})
	, _inboundProcessor(InboundMessageProcessor::Dependencies{
		&_framework,
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
	_animationRegistry.Clear();
	_lastTickTime = {};

	if (!InitializeGameplayContent() ||
		!InitializeFrameworkRuntime() ||
		!InitializeNetworkRuntime())
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

	std::cout << "[ServerApp] ServerApp Initialize Success.\n";

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
	_animationRegistry.Clear();
	_lastTickTime = {};
}

TransferId ServerApp::RequestSessionWorldTransfer(
	SessionId sessionId,
	WorldDefId targetWorldDefId,
	uint64_t instanceKey,
	PartyId partyId,
	bool allowFallback)
{
	if (!IsInitialized() || sessionId == 0 || targetWorldDefId == WorldDefId::None)
	{
		return 0;
	}

	const WorldId sourceWorldId =
		_sessionBindings.FindCurrentWorldId(sessionId);
	if (!sourceWorldId.IsValid())
	{
		std::cout << "[ServerApp] world transfer rejected: no source binding."
			<< " sessionId=" << sessionId
			<< " targetDefId=" << static_cast<int>(targetWorldDefId)
			<< "\n";
		return 0;
	}

	const SessionId sessions[] = { sessionId };

	return _framework.RequestWorldTransfer(
		std::span<const SessionId>(sessions, 1),
		sourceWorldId,
		targetWorldDefId,
		instanceKey,
		partyId,
		allowFallback,
		_nowSec);
}

TransferId ServerApp::RequestDebugWorldTransfer(
	SessionId sessionId,
	WorldDefId targetWorldDefId,
	uint64_t instanceKey,
	bool allowFallback)
{
	const uint64_t resolvedInstanceKey =
		instanceKey != 0 ? instanceKey : static_cast<uint64_t>(sessionId);
	const PartyId partyId = static_cast<PartyId>(resolvedInstanceKey);
	return RequestSessionWorldTransfer(
		sessionId,
		targetWorldDefId,
		resolvedInstanceKey,
		partyId,
		allowFallback);
}

TransferId ServerApp::RequestDebugTransferToVillage(SessionId sessionId)
{
	return RequestDebugWorldTransfer(sessionId, WorldDefId::Village);
}

bool ServerApp::InitializeFrameworkRuntime()
{
	_bootstrapFactory.SetAnimationRegistry(&_animationRegistry);
	_bootstrapFactory.SetFramework(&_framework);
	_bootstrapFactory.SetBootstrapWorldId(&_startupWorldId);

	FrameworkRuntime::BootstrapParams bootstrapParams{};
	bootstrapParams.worldFactory = &_bootstrapFactory;
	bootstrapParams.definitionProvider = &_bootstrapDefinitions;
	bootstrapParams.transferBinding = &_transferBinding;

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
	_animationRegistry.Clear();

	const std::filesystem::path animationRoot =
		ServerPathResolver::GetDefaultAnimationOutputRoot();
	std::cout << "[ServerApp] Animation content root="
		<< animationRoot.string() << std::endl;

	const std::vector<std::filesystem::path> candidates =
		ServerPathResolver::GetBootAnimationCandidates(animationRoot);

	std::vector<AnimationClipDef> animationDefs;
	for (const auto& path : candidates)
	{
		if (!std::filesystem::exists(path))
		{
			continue;
		}

		AnimationClipDef def;
		const auto loadResult = _animationLoader.LoadFile(path, def);
		if (!loadResult.succeeded)
		{
			std::cout << "[ServerApp] Animation load failed."
				<< " path=" << path.string()
				<< " error=" << loadResult.error << "\n";
			return false;
		}

		animationDefs.push_back(std::move(def));
	}

	if (animationDefs.empty())
	{
		std::cout << "[ServerApp] No animation json files were loaded."
			<< " root=" << animationRoot.string() << "\n";
		return false;
	}

	const AnimationRegistry::BuildResult buildResult =
		_animationRegistry.Rebuild(std::move(animationDefs));
	if (!buildResult.succeeded)
	{
		std::cout << "[ServerApp] Animation registry build failed."
			<< " error=" << buildResult.error << "\n";
		return false;
	}

	std::cout << "[ServerApp] Animation content loaded."
		<< " clips=" << buildResult.loadedCount << "\n";
	return true;
}

bool ServerApp::InitializeStartupWorld()
{
	// Plaza is currently modeled as a pre-created persistent hub world.
	constexpr WorldDefId startupWorldDefId = WorldDefId::Plaza;
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
	ServerDirtyReplicationService::BuildAndStage(
		_framework,
		_network,
		_sessionBindings);
	FlushOutbound();

	++_tickCount;
}

void ServerApp::DrainInboundCommands()
{
	_network.DrainInboundMessages(_inboundMessages);
}

void ServerApp::ProcessInboundMessages()
{
	_inboundProcessor.Process(_inboundMessages, _remainingInboundMessages);
	_inboundMessages.swap(_remainingInboundMessages);
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

	if (!ServerWorldTransferCommitter::Commit(_framework, _sessionBindings))
	{
		std::cout << "[ServerApp] World transfer commit failed.\n";
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

	if (!ServerFrameEventDispatcher::Dispatch(
		frameResult,
		_framework,
		_network,
		_sessionBindings,
		_playerEntryService,
		_nowSec))
	{
		std::cout << "[ServerApp] Frame event dispatch failed.\n";
		Stop();
		return;
	}

	++_frameIndex;
}

void ServerApp::FlushOutbound()
{
	_network.FlushSendStage();
}
