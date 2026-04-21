#include "pch.h"
#include "ServerApp.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <span>
#include <thread>
#include <stdlib.h>

#include "ServerDirtyReplicationService.h"
#include "ServerFrameEventDispatcher.h"
#include "ServerPacketStager.h"
#include "ServerPathResolver.h"
#include "ServerReplicationSnapshot.h"
#include "ServerWorldTransferCommitter.h"
#include "WorldInstanceRecord.h"

namespace
{
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
		this,
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
	_worldTransitionRequestIds.clear();
	_pendingClientTransitions.clear();
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
	_worldTransitionRequestIds.clear();
	_pendingClientTransitions.clear();
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

TransferId ServerApp::RequestDemoWorldTransition(
	SessionId sessionId,
	uint32_t requestId)
{
	if (!IsInitialized() || sessionId == 0)
	{
		return 0;
	}

	const WorldId sourceWorldId =
		_sessionBindings.FindCurrentWorldId(sessionId);
	const WorldInstanceRecord* const sourceRecord =
		_framework.FindWorldRecord(sourceWorldId);
	if (sourceRecord == nullptr)
	{
		std::cout << "[ServerApp] demo world transition rejected: source world not found."
			<< " sessionId=" << sessionId
			<< " requestId=" << requestId
			<< "\n";
		return 0;
	}

	const WorldDefId targetWorldDefId =
		ResolveDemoNextWorld(sourceRecord->defId);
	if (targetWorldDefId == WorldDefId::None)
	{
		std::cout << "[ServerApp] demo world transition rejected: no next world."
			<< " sessionId=" << sessionId
			<< " requestId=" << requestId
			<< " sourceDefId=" << static_cast<int>(sourceRecord->defId)
			<< "\n";
		return 0;
	}

	if (_pendingClientTransitions.contains(sessionId))
	{
		std::cout << "[ServerApp] demo world transition rejected: client transition pending."
			<< " sessionId=" << sessionId
			<< " requestId=" << requestId
			<< "\n";
		return 0;
	}

	std::vector<SessionId> sourceSessionIds;
	_sessionBindings.CollectSessionsInWorld(sourceWorldId, sourceSessionIds);
	if (std::find(sourceSessionIds.begin(), sourceSessionIds.end(), sessionId) ==
		sourceSessionIds.end())
	{
		std::cout << "[ServerApp] demo world transition rejected: session not in source world."
			<< " sessionId=" << sessionId
			<< " requestId=" << requestId
			<< "\n";
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
	const auto it = _pendingClientTransitions.find(sessionId);
	if (it == _pendingClientTransitions.end())
	{
		std::cout << "[ServerApp] client world transition ready rejected: no pending transition."
			<< " sessionId=" << sessionId
			<< " transferId=" << transferId
			<< "\n";
		return false;
	}

	const PendingClientTransition& pending = it->second;
	if (pending.transferId != transferId)
	{
		std::cout << "[ServerApp] client world transition ready rejected: transfer mismatch."
			<< " sessionId=" << sessionId
			<< " expectedTransferId=" << pending.transferId
			<< " actualTransferId=" << transferId
			<< "\n";
		return false;
	}

	ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
		_framework,
		_network,
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
			_sessionBindings.CollectSessionsInWorld(
				pending.targetWorldId,
				worldSessionIds);

			otherReadySessionIds.reserve(worldSessionIds.size());
			for (SessionId worldSessionId : worldSessionIds)
			{
				if (worldSessionId == sessionId ||
					_pendingClientTransitions.contains(worldSessionId))
				{
					continue;
				}

				otherReadySessionIds.push_back(worldSessionId);
			}

			(void)ServerPacketStager::StageSpawnAddPacketToSessions(
				_network,
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

	std::vector<SessionId> pendingTransitionSessions;
	pendingTransitionSessions.reserve(_pendingClientTransitions.size());
	for (const auto& [sessionId, pending] : _pendingClientTransitions)
	{
		(void)pending;
		pendingTransitionSessions.push_back(sessionId);
	}

	ServerDirtyReplicationService::BuildAndStage(
		_framework,
		_network,
		_sessionBindings,
		std::span<const SessionId>(
			pendingTransitionSessions.data(),
			pendingTransitionSessions.size()));
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

	WorldTransferEventBatch transferEvents{};
	_framework.DrainWorldTransferEvents(transferEvents);

	if (!ServerWorldTransferCommitter::Commit(
		_framework,
		_sessionBindings,
		transferEvents))
	{
		std::cout << "[ServerApp] World transfer commit failed.\n";
		Stop();
		return;
	}

	if (!StageWorldTransitionBeginPackets(transferEvents))
	{
		std::cout << "[ServerApp] World transition begin staging failed.\n";
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

	std::vector<SessionId> pendingTransitionSessions;
	pendingTransitionSessions.reserve(_pendingClientTransitions.size());
	for (const auto& [sessionId, pending] : _pendingClientTransitions)
	{
		(void)pending;
		pendingTransitionSessions.push_back(sessionId);
	}

	if (!ServerFrameEventDispatcher::Dispatch(
		frameResult,
		_framework,
		_network,
		_sessionBindings,
		_playerEntryService,
		_nowSec,
		std::span<const SessionId>(
			pendingTransitionSessions.data(),
			pendingTransitionSessions.size())))
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
			std::cout << "[ServerApp] World transition begin failed: missing world def."
				<< " transferId=" << completed.transferId
				<< " sourceWorldId=" << completed.sourceWorldId.GetRaw()
				<< " targetWorldId=" << completed.targetWorldId.GetRaw()
				<< "\n";
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
				_network,
				imported.sessionId,
				transition))
			{
				std::cout << "[ServerApp] World transition begin packet stage failed."
					<< " transferId=" << completed.transferId
					<< " sessionId=" << imported.sessionId
					<< "\n";
				return false;
			}

			_pendingClientTransitions[imported.sessionId] =
				PendingClientTransition{
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
