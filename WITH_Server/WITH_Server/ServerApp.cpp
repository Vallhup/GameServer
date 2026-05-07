#include "pch.h"
#include "ServerApp.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <span>
#include <thread>
#include <stdlib.h>

#include "FrameworkLog.h"

#include "ServerDirtyReplicationService.h"
#include "ServerFrameEventDispatcher.h"
#include "ServerPacketStager.h"
#include "ServerPathResolver.h"
#include "ServerReplicationSnapshot.h"
#include "ServerWorldTransferCommitter.h"
#include "AIBehaviorDef.h"
#include "CharacterDef.h"
#include "GameDataCatalog.h"
#include "GameplayDefValidator.h"
#include "SpawnSetDef.h"
#include "WorldInstanceRecord.h"

namespace
{
	constexpr const char* kLogCategory = "ServerApp";
	constexpr uint32_t kWorldTransitionReasonDebug = 1;
	constexpr size_t kDemoPartyMaxMembers = 3;

	WorldDefId ResolveDemoNextWorld(WorldDefId currentWorldDefId) noexcept
	{
		switch (currentWorldDefId) {
		case WorldDefId::Plaza:
			return WorldDefId::Village;
		case WorldDefId::Village:
			return WorldDefId::Castle;
		case WorldDefId::Castle:
			return WorldDefId::Final;
		case WorldDefId::Final:
		case WorldDefId::Pvp:
			return WorldDefId::Plaza;
		default:
			return WorldDefId::None;
		}
	}

	PartyId MakeDemoPartyId(std::span<const SessionId> sessionIds) noexcept
	{
		if (sessionIds.empty())
		{
			return 0;
		}

		const auto minIt = std::min_element(sessionIds.begin(), sessionIds.end());
		return static_cast<PartyId>(*minIt);
	}
}

ServerApp::ServerApp(Config config)
	: _config(config)
	, _framework(FrameworkRuntime::Config{
		.executorWorkerCount = _config.executorWorkerCount,
		.executorDiagnostics = TaskExecutorDiagnosticsConfig{
			.enabled = true,
			.collectNodeTimings = true,
			.logFrameSummary = true,
			.writeCsv = true,
			.sampleEveryNFrames = 60,
			.summaryCsvPath = "Log/WITH_Server_ExecPerfSummary.csv",
			.nodeCsvPath = "Log/WITH_Server_ExecPerfNodes.csv"
		}
	})
	, _sessionSystem(ServerSessionSystem::Config{
		.networkThreadCount = _config.networkThreadCount,
		.listenPort = _config.listenPort,
		.maxSessions = _config.maxSessions
	}, _framework, _startupWorldId, *this)
	, _transferBinding(_framework, _sessionSystem.Bindings())
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
	_sessionSystem.ClearSessionState();
	_worldTransitionRequestIds.clear();
	_pendingClientTransitions.clear();
	_animationRegistry.Clear();
	_lastTickTime = {};

	if (!InitializeGameplayContent() ||
		!InitializeFrameworkRuntime() ||
		!InitializeSessionSystem())
	{
		_sessionSystem.Shutdown();
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

	FWLOG_INFO(kLogCategory, "Initialize success");

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
	_sessionSystem.Shutdown();
	_framework.Shutdown();
	_initialized.store(false);
	_tickCount = 0;
	_frameIndex = 1;
	_nowSec = 0.0;
	_startupWorldId = WorldId::Invalid();
	_sessionSystem.ClearSessionState();
	_worldTransitionRequestIds.clear();
	_pendingClientTransitions.clear();
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
		_sessionSystem.Bindings().FindCurrentWorldId(sessionId);
	if (!sourceWorldId.IsValid())
	{
		FWLOG_WARN(kLogCategory, "World transfer rejected: no source binding (sid=%u, targetDefId=%d)",
			sessionId, static_cast<int>(targetWorldDefId));
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

TransferId ServerApp::RequestDemoWorldTransition(
	SessionId sessionId,
	uint32_t requestId)
{
	if (!IsInitialized() || sessionId == 0)
	{
		return 0;
	}

	const WorldId sourceWorldId =
		_sessionSystem.Bindings().FindCurrentWorldId(sessionId);
	const WorldInstanceRecord* const sourceRecord =
		_framework.FindWorldRecord(sourceWorldId);
	if (sourceRecord == nullptr)
	{
		FWLOG_WARN(kLogCategory, "Demo world transition rejected: source world not found (sid=%u, requestId=%u)",
			sessionId, requestId);
		return 0;
	}

	const WorldDefId targetWorldDefId =
		ResolveDemoNextWorld(sourceRecord->defId);
	if (targetWorldDefId == WorldDefId::None)
	{
		FWLOG_WARN(kLogCategory, "Demo world transition rejected: no next world (sid=%u, requestId=%u, sourceDefId=%d)",
			sessionId, requestId, static_cast<int>(sourceRecord->defId));
		return 0;
	}

	if (_pendingClientTransitions.contains(sessionId))
	{
		FWLOG_WARN(kLogCategory, "Demo world transition rejected: client transition pending (sid=%u, requestId=%u)",
			sessionId, requestId);
		return 0;
	}

	std::vector<SessionId> sourceSessionIds;
	_sessionSystem.Bindings().CollectSessionsInWorld(sourceWorldId, sourceSessionIds);
	if (std::find(sourceSessionIds.begin(), sourceSessionIds.end(), sessionId) ==
		sourceSessionIds.end())
	{
		FWLOG_WARN(kLogCategory, "Demo world transition rejected: session not in source world (sid=%u, requestId=%u)",
			sessionId, requestId);
		return 0;
	}

	std::vector<SessionId> transferSessionIds;
	transferSessionIds.reserve(kDemoPartyMaxMembers);
	transferSessionIds.push_back(sessionId);
	for (SessionId candidateSessionId : sourceSessionIds)
	{
		if (candidateSessionId == sessionId ||
			_pendingClientTransitions.contains(candidateSessionId))
		{
			continue;
		}

		transferSessionIds.push_back(candidateSessionId);
		if (transferSessionIds.size() >= kDemoPartyMaxMembers)
		{
			break;
		}
	}

	std::sort(transferSessionIds.begin(), transferSessionIds.end());

	const PartyId partyId = MakeDemoPartyId(transferSessionIds);
	const uint64_t instanceKey =
		targetWorldDefId == WorldDefId::Plaza ? 0 : static_cast<uint64_t>(partyId);
	const TransferId transferId = _framework.RequestWorldTransfer(
		std::span<const SessionId>(
			transferSessionIds.data(),
			transferSessionIds.size()),
		sourceWorldId,
		targetWorldDefId,
		instanceKey,
		partyId,
		true,
		_nowSec);
	if (transferId != 0)
	{
		_worldTransitionRequestIds[transferId][sessionId] = requestId;
	}

	return transferId;
}

bool ServerApp::MarkClientWorldTransitionReady(
	SessionId sessionId,
	TransferId transferId)
{
	std::vector<SessionId> pendingTransitionSessions;
	pendingTransitionSessions.reserve(_pendingClientTransitions.size());
	for (const auto& [pendingSessionId, pending] : _pendingClientTransitions)
	{
		(void)pending;
		pendingTransitionSessions.push_back(pendingSessionId);
	}

	const InitialWorldReadyResult initialReadyResult =
		_sessionSystem.MarkInitialWorldReady(
			sessionId,
			transferId,
			_nowSec,
			std::span<const SessionId>(
				pendingTransitionSessions.data(),
				pendingTransitionSessions.size()));
	if (initialReadyResult == InitialWorldReadyResult::Accepted)
	{
		return true;
	}
	if (initialReadyResult == InitialWorldReadyResult::Rejected)
	{
		return false;
	}

	const auto it = _pendingClientTransitions.find(sessionId);
	if (it == _pendingClientTransitions.end())
	{
		FWLOG_WARN(kLogCategory, "Client world transition ready rejected: no pending transition (sid=%u, transferId=%u)",
			sessionId, transferId);
		return false;
	}

	const PendingClientTransition& pending = it->second;
	if (pending.transferId != transferId)
	{
		FWLOG_WARN(kLogCategory, "Client world transition ready rejected: transfer mismatch (sid=%u, expected=%u, actual=%u)",
			sessionId, pending.transferId, transferId);
		return false;
	}

	ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
		_framework,
		_sessionSystem.Network(),
		pending.targetWorldId,
		sessionId,
		NetId::Invalid());

	// Transfer-imported players skip the normal EntitySpawned broadcast path,
	// so notify already-ready sessions in the target world explicitly here.
	const NetBindingLocation playerBinding =
		_framework.FindNetBinding(pending.playerNetId);
	if (playerBinding.IsValid() &&
		playerBinding.worldId == pending.targetWorldId)
	{
		CharacterId characterId = CharacterId::None;
		const WorldTransformComp* transform = nullptr;
		if (ServerReplicationSnapshot::TryGetReplicatedSpawnState(
			_framework,
			pending.targetWorldId,
			playerBinding.entity,
			characterId,
			transform))
		{
			std::vector<SessionId> worldSessionIds;
			std::vector<SessionId> otherReadySessionIds;
			_sessionSystem.Bindings().CollectSessionsInWorld(
				pending.targetWorldId,
				worldSessionIds);
			_sessionSystem.AppendPendingInitialEntrySessions(
				pendingTransitionSessions);

			otherReadySessionIds.reserve(worldSessionIds.size());
			for (SessionId worldSessionId : worldSessionIds)
			{
				if (worldSessionId == sessionId ||
					std::find(
						pendingTransitionSessions.begin(),
						pendingTransitionSessions.end(),
						worldSessionId) != pendingTransitionSessions.end())
				{
					continue;
				}

				otherReadySessionIds.push_back(worldSessionId);
			}

			(void)ServerPacketStager::StageSpawnAddPacketToSessions(
				_sessionSystem.Network(),
				std::span<const SessionId>(
					otherReadySessionIds.data(),
					otherReadySessionIds.size()),
				pending.playerNetId,
				characterId,
				transform);
		}
	}

	_pendingClientTransitions.erase(it);
	return true;
}

bool ServerApp::InitializeFrameworkRuntime()
{
	_bootstrapFactory.SetAnimationRegistry(&_animationRegistry);
	_bootstrapFactory.SetFramework(&_framework);
	_bootstrapFactory.SetBootstrapWorldId(&_startupWorldId);
	_bootstrapFactory.SetGameDataCatalog(&_gameDataCatalog);

	FrameworkRuntime::BootstrapParams bootstrapParams{};
	bootstrapParams.worldFactory = &_bootstrapFactory;
	bootstrapParams.definitionProvider = &_bootstrapDefinitions;
	bootstrapParams.transferBinding = &_transferBinding;

	if (!_framework.Initialize(bootstrapParams))
	{
		FWLOG_FATAL(kLogCategory, "Framework bootstrap failed");
		return false;
	}

	return InitializeStartupWorld();
}

bool ServerApp::InitializeSessionSystem()
{
	return _sessionSystem.Initialize();
}

bool ServerApp::InitializeGameplayContent()
{
	_animationRegistry.Clear();

	const GameplayContentCatalogRoots gameplayContentRoots
	{
		.attributeRoot = ServerPathResolver::GetDefaultAttributeDefRoot(),
		.tagRoot = ServerPathResolver::GetDefaultGameplayTagDefRoot(),
		.effectRoot = ServerPathResolver::GetDefaultGameplayEffectDefRoot(),
		.abilityRoot = ServerPathResolver::GetDefaultAbilityDefRoot(),
		.abilitySetRoot = ServerPathResolver::GetDefaultAbilitySetDefRoot()
	};
	const DefLoadResult gameplayContentLoadResult =
		_gameplayContentCatalog.LoadFromRoots(gameplayContentRoots);
	if (!gameplayContentLoadResult.succeeded)
	{
		FWLOG_FATAL(kLogCategory, "Gameplay content catalog load failed (error=%s)",
			gameplayContentLoadResult.error.c_str());
		return false;
	}

	FWLOG_INFO(kLogCategory, "Gameplay content catalog loaded (count=%zu)",
		gameplayContentLoadResult.loadedCount);
	GameplayContentCatalogSnapshot::Publish(_gameplayContentCatalog);

	const GameDataCatalogRoots gameDataRoots
	{
		.characterRoot = ServerPathResolver::GetDefaultCharacterDefRoot(),
		.aiBehaviorRoot = ServerPathResolver::GetDefaultAIBehaviorDefRoot(),
		.spawnSetRoot = ServerPathResolver::GetDefaultSpawnSetDefRoot()
	};
	const DefLoadResult gameDataLoadResult =
		_gameDataCatalog.LoadFromRoots(gameDataRoots);
	if (!gameDataLoadResult.succeeded)
	{
		FWLOG_FATAL(kLogCategory, "Game data catalog load failed (error=%s)",
			gameDataLoadResult.error.c_str());
		return false;
	}

	FWLOG_INFO(kLogCategory, "Game data catalog loaded (count=%zu)",
		gameDataLoadResult.loadedCount);
	GameDataCatalog::Publish(_gameDataCatalog);

	const DefLoadResult validationResult =
		GamePlayDefValidator::ValidateGameplayDefs(_gameDataCatalog);
	if (!validationResult.succeeded)
	{
		FWLOG_FATAL(kLogCategory, "Gameplay defs validation failed (error=%s)",
			validationResult.error.c_str());
		return false;
	}

	FWLOG_INFO(kLogCategory, "Gameplay defs validated (count=%zu)", validationResult.loadedCount);

	const std::filesystem::path animationRoot =
		ServerPathResolver::GetDefaultAnimationOutputRoot();
	FWLOG_DEBUG(kLogCategory, "Animation content root=%s", animationRoot.string().c_str());

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
			FWLOG_FATAL(kLogCategory, "Animation load failed (path=%s, error=%s)",
				path.string().c_str(), loadResult.error.c_str());
			return false;
		}

		animationDefs.push_back(std::move(def));
	}

	if (animationDefs.empty())
	{
		FWLOG_FATAL(kLogCategory, "No animation json files were loaded (root=%s)",
			animationRoot.string().c_str());
		return false;
	}

	const AnimationRegistry::BuildResult buildResult =
		_animationRegistry.Rebuild(std::move(animationDefs));
	if (!buildResult.succeeded)
	{
		FWLOG_FATAL(kLogCategory, "Animation registry build failed (error=%s)",
			buildResult.error.c_str());
		return false;
	}

	FWLOG_INFO(kLogCategory, "Animation content loaded (clips=%zu)", buildResult.loadedCount);
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
		FWLOG_FATAL(kLogCategory, "Startup world creation failed (defId=%d, instanceKey=%llu)",
			static_cast<int>(startupWorldDefId), startupInstanceKey);
		return false;
	}

	if (!_framework.InitializeWorld(_startupWorldId))
	{
		FWLOG_FATAL(kLogCategory, "Startup world initialization failed (worldId=%u)",
			_startupWorldId.GetRaw());
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

	_sessionSystem.BeginSendStage();
	RunWorldFrames(dtSec);

	std::vector<SessionId> pendingTransitionSessions;
	pendingTransitionSessions.reserve(_pendingClientTransitions.size());
	for (const auto& [sessionId, pending] : _pendingClientTransitions)
	{
		(void)pending;
		pendingTransitionSessions.push_back(sessionId);
	}
	_sessionSystem.AppendPendingInitialEntrySessions(pendingTransitionSessions);

	ServerDirtyReplicationService::BuildAndStage(
		_framework,
		_sessionSystem.Network(),
		_sessionSystem.Bindings(),
		std::span<const SessionId>(
			pendingTransitionSessions.data(),
			pendingTransitionSessions.size()));
	FlushOutbound();

	++_tickCount;
}

void ServerApp::RunWorldFrames(double dtSec)
{
	_nowSec += dtSec;

	if (!_framework.TickServices(_nowSec, dtSec))
	{
		FWLOG_ERROR(kLogCategory, "Framework service tick failed");
		Stop();
		return;
	}

	WorldTransferEventBatch transferEvents{};
	_framework.DrainWorldTransferEvents(transferEvents);

	if (!ServerWorldTransferCommitter::Commit(
		_framework,
		_sessionSystem.Bindings(),
		transferEvents))
	{
		FWLOG_ERROR(kLogCategory, "World transfer commit failed");
		Stop();
		return;
	}

	if (!StageWorldTransitionBeginPackets(transferEvents))
	{
		FWLOG_ERROR(kLogCategory, "World transition begin staging failed");
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
		FWLOG_ERROR(kLogCategory, "Frame execution failed (frameIndex=%llu, reason=%d, selectedWorlds=%u)",
			_frameIndex, static_cast<int>(frameResult.failureReason), frameResult.selectedWorldCount);
		Stop();
		return;
	}

	std::vector<SessionId> pendingTransitionSessions;
	pendingTransitionSessions.reserve(_pendingClientTransitions.size());
	for (const auto& [sessionId, pending] : _pendingClientTransitions)
	{
		(void)pending;
		pendingTransitionSessions.push_back(sessionId);
	}
	_sessionSystem.AppendPendingInitialEntrySessions(pendingTransitionSessions);

	if (!ServerFrameEventDispatcher::Dispatch(
		frameResult,
		_framework,
		_sessionSystem,
		_nowSec,
		std::span<const SessionId>(
			pendingTransitionSessions.data(),
			pendingTransitionSessions.size())))
	{
		FWLOG_ERROR(kLogCategory, "Frame event dispatch failed");
		Stop();
		return;
	}

	++_frameIndex;
}

void ServerApp::FlushOutbound()
{
	_sessionSystem.FlushOutbound();
}

bool ServerApp::StageWorldTransitionBeginPackets(
	const WorldTransferEventBatch& transferEvents)
{
	for (const WorldTransferCompletedEvent& completed : transferEvents.completed)
	{
		const WorldInstance* const targetWorld =
			_framework.FindWorld(completed.targetWorldId);
		const WorldInstance* const sourceWorld =
			_framework.FindWorld(completed.sourceWorldId);
		if (targetWorld == nullptr ||
			targetWorld->GetDef() == nullptr ||
			sourceWorld == nullptr ||
			sourceWorld->GetDef() == nullptr)
		{
			FWLOG_ERROR(kLogCategory, "World transition begin failed: missing world def (transferId=%u, sourceWorldId=%u, targetWorldId=%u)",
				completed.transferId, completed.sourceWorldId.GetRaw(), completed.targetWorldId.GetRaw());
			return false;
		}

		const auto requestIt =
			_worldTransitionRequestIds.find(completed.transferId);

		const WorldDef* const sourceDef = sourceWorld->GetDef();
		const WorldDef* const targetDef = targetWorld->GetDef();

		for (const ImportedTransferEntity& imported : completed.importedEntities)
		{
			uint32_t requestId = 0;
			if (requestIt != _worldTransitionRequestIds.end())
			{
				const auto sessionRequestIt = requestIt->second.find(imported.sessionId);
				if (sessionRequestIt != requestIt->second.end())
				{
					requestId = sessionRequestIt->second;
				}
			}

			ServerWorldTransitionBeginPacket transition{};
			transition.transferId = completed.transferId;
			transition.requestId = requestId;
			transition.sourceWorldDefId = static_cast<uint32_t>(sourceDef->id);
			transition.sourceWorldId = completed.sourceWorldId.GetRaw();
			transition.targetWorldDefId = static_cast<uint32_t>(targetDef->id);
			transition.targetWorldId = completed.targetWorldId.GetRaw();
			transition.mapResourceId = targetDef->map.resourceId;
			transition.playerNetId = imported.netId.GetRaw();
			transition.clearExistingObjects = true;
			transition.waitClientReady = true;
			transition.usedFallback = completed.usedFallback;
			transition.reason = kWorldTransitionReasonDebug;

			if (!ServerPacketStager::StageWorldTransitionBeginPacket(
				_sessionSystem.Network(),
				imported.sessionId,
				transition))
			{
				FWLOG_ERROR(kLogCategory, "World transition begin packet stage failed (transferId=%u, sid=%u)",
					completed.transferId, imported.sessionId);
				return false;
			}

			_pendingClientTransitions[imported.sessionId] =
				PendingClientTransition
				{
					.transferId = completed.transferId,
					.sessionId = imported.sessionId,
					.targetWorldId = completed.targetWorldId,
					.playerNetId = imported.netId,
					.mapResourceId = targetDef->map.resourceId
				};
		}

		if (requestIt != _worldTransitionRequestIds.end())
		{
			_worldTransitionRequestIds.erase(requestIt);
		}
	}

	return true;
}
