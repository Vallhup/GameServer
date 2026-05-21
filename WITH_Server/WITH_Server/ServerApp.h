#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "FrameworkRuntime.h"
#include "GameDataCatalog.h"
#include "GameplayContentCatalog.h"
#include "AnimationJsonLoader.h"
#include "AnimationRegistry.h"
#include "DemoPartyFormationPolicy.h"
#include "IWorldTransitionRequestSink.h"
#include "ODBCDatabaseBackend.h"
#include "PartyCommandPump.h"
#include "PartyCommandQueue.h"
#include "PartyService.h"
#include "ServerSessionSystem.h"
#include "ServerWorldBootstrap.h"
#include "ServerWorldTransferBinding.h"

class ServerApp final
	: public IWorldTransitionRequestSink
	, private IPartySessionQuery
	, private IDemoPartyTransitionQuery {
public:
	struct Config
	{
		uint16_t networkThreadCount{ 4 };
		uint16_t listenPort{ 7000 };

		uint32_t logicTickHz{ 30 };
		uint32_t executorWorkerCount{ 4 };

		uint32_t maxSessions{ 1024 };
		uint32_t maxWorlds{ 128 };

		ODBCDatabaseBackend::Config database;
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

	TransferId RequestSessionWorldTransfer(
		SessionId sessionId,
		WorldDefId targetWorldDefId,
		uint64_t instanceKey,
		PartyId partyId,
		bool allowFallback);

	TransferId RequestDebugWorldTransfer(
		SessionId sessionId,
		WorldDefId targetWorldDefId,
		uint64_t instanceKey = 0,
		bool allowFallback = true);

	TransferId RequestDebugTransferToVillage(SessionId sessionId);

	TransferId RequestDemoWorldTransition(
		SessionId sessionId,
		uint32_t requestId) override;

	bool MarkClientWorldTransitionReady(
		SessionId sessionId,
		TransferId transferId) override;

	void OnSessionDisconnected(SessionId sessionId) noexcept override;

private:
	struct PendingClientTransition
	{
		TransferId transferId{ 0 };
		SessionId sessionId{ 0 };
		WorldId targetWorldId{ WorldId::Invalid() };
		NetId playerNetId{ NetId::Invalid() };
		uint32_t mapResourceId{ 0 };
	};

private:
	bool InitializeFrameworkRuntime();
	bool InitializeDatabaseRuntime();
	bool InitializeSessionSystem();
	bool InitializeGameplayContent();
	bool InitializeStartupWorld();
	void ShutdownDatabaseRuntime() noexcept;

	void RunLogicLoop();
	void TickOnce(double dtSec);

	void RunWorldFrames(double dtSec);
	void FlushOutbound();
	bool StageWorldTransitionBeginPackets(
		const WorldTransferEventBatch& transferEvents);
	void ApplyPartyWorldTransferEvents(
		const WorldTransferEventBatch& transferEvents);

	bool IsPartyEligible(SessionId sessionId) const override;
	uint64_t FindAccountId(SessionId sessionId) const override;
	NetId FindControlledNetId(SessionId sessionId) const override;
	WorldId FindCurrentWorldId(SessionId sessionId) const override;
	void CollectSessionsInWorld(
		WorldId worldId,
		std::vector<SessionId>& outSessionIds) const override;
	bool CanBeginWorldTransfer(SessionId sessionId) const override;
	bool IsClientTransitionPending(SessionId sessionId) const override;

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
	GameDataCatalog _gameDataCatalog;
	GameplayContentCatalogSnapshot _gameplayContentCatalog;
	WorldId _startupWorldId{};
	std::unique_ptr<ODBCDatabaseBackend> _databaseBackend;
	ServerSessionSystem _sessionSystem;
	ServerWorldTransferBinding _transferBinding;
	PartyService _partyService;
	PartyCommandQueue _partyCommandQueue;
	PartyCommandPump _partyCommandPump;
	DemoPartyFormationPolicy _demoPartyPolicy;
	std::unordered_map<TransferId, std::unordered_map<SessionId, uint32_t>>
		_worldTransitionRequestIds;
	std::unordered_map<SessionId, PendingClientTransition> _pendingClientTransitions;

	uint64_t _tickCount{ 0 };
	uint64_t _frameIndex{ 0 };
	double _nowSec{ 0.0 };
	std::chrono::steady_clock::time_point _lastTickTime{};
};
