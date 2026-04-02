#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

#include "FrameworkRuntime.h"
#include "AnimationJsonLoader.h"
#include "AnimationRegistry.h"
#include "InboundMessageProcessor.h"
#include "NetworkRuntime.h"
#include "PlayerEntryService.h"
#include "SessionBindingRegistry.h"
#include "ServerWorldBootstrap.h"

class ServerApp final {
public:
	struct Config
	{
		uint16_t networkThreadCount{ 4 };
		uint16_t listenPort{ 7000 };

		uint32_t logicTickHz{ 30 };
		uint32_t executorWorkerCount{ 4 };

		uint32_t maxSessions{ 1024 };
		uint32_t maxWorlds{ 128 };
	};

public:
	explicit ServerApp(Config config = {});
	~ServerApp() = default;

	ServerApp(const ServerApp&) = delete;
	ServerApp& operator=(const ServerApp&) = delete;

	bool Initialize();
	void Run();
	void Stop() noexcept;
	void Shutdown() noexcept;

	bool IsInitialized() const noexcept { return _initialized.load(); }
	bool IsRunning() const noexcept { return _running.load(); }

	const Config& GetConfig() const noexcept { return _config; }
	uint64_t TickCount() const noexcept { return _tickCount; }
	const AnimationRegistry& GetAnimationRegistry() const noexcept { return _animationRegistry; }

private:
	bool InitializeFrameworkRuntime();
	bool InitializeNetworkRuntime();
	bool InitializeGameplayContent();
	bool InitializeStartupWorld();

	void RunLogicLoop();
	void TickOnce(double dtSec);

	void DrainInboundCommands();
	void ProcessInboundMessages();
	void RunWorldFrames(double dtSec);
	void FinalizeFrameEvents(const FrameworkRuntime::FrameResult& frameResult);
	void BuildReplication();
	void FlushOutbound();

private:
	Config _config;

	std::atomic<bool> _initialized{ false };
	std::atomic<bool> _running{ false };
	std::atomic<bool> _stopRequested{ false };

	ServerWorldBootstrapFactory _bootstrapFactory;
	ServerWorldBootstrapDefinitionProvider _bootstrapDefinitions;
	FrameworkRuntime _framework;
	AnimationJsonLoader _animationLoader;
	AnimationRegistry _animationRegistry;
	NetworkRuntime _network;
	WorldId _startupWorldId{};
	SessionBindingRegistry _sessionBindings;
	PlayerEntryService _playerEntryService;
	InboundMessageProcessor _inboundProcessor;
	std::vector<InboundMessage> _inboundMessages;

	uint64_t _tickCount{ 0 };
	uint64_t _frameIndex{ 0 };
	double _nowSec{ 0.0 };
	std::chrono::steady_clock::time_point _lastTickTime{};
};
