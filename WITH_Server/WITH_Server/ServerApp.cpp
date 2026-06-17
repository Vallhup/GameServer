#include "pch.h"
#include "ServerApp.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <thread>
#include <stdlib.h>

#include "FrameworkLog.h"

#include "ECS/GameplayRuntimeComponents.h"
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
#include "ECS/System/Phase8/PlayerDeathStatePolicy.h"
#include "SpawnSetDef.h"
#include "WorldInstanceRecord.h"

namespace
{
	constexpr const char* kLogCategory = "ServerApp";
	constexpr uint32_t kWorldTransitionReasonDebug = 1;
	constexpr double kFinalClearChoiceTimeoutSec = 15.0;
	constexpr double kBeaconCinematicStartGuardSec = 30.0;
	constexpr float kBeaconInteractionServerRadius = 3.0f;
	// 엔딩/페이드 연출 완료를 기다리는 안전 타임아웃. 일부 클라가 연출 완료를
	// 보고하지 않아도 이 시간 후에는 강제로 Plaza 전이한다.
	constexpr double kEndingCinematicTimeoutSec = 180.0;

	struct BeaconCinematicPolicy
	{
		Protocol::BeaconCinematicType cinematicType{
			Protocol::BEACON_CINEMATIC_TYPE_UNSPECIFIED };
		float interactionX{ 0.0f };
		float interactionZ{ 0.0f };
	};

	std::optional<BeaconCinematicPolicy> ResolveBeaconCinematicPolicy(
		WorldDefId worldDefId) noexcept
	{
		switch (worldDefId)
		{
		case WorldDefId::Village:
			return BeaconCinematicPolicy{
				.cinematicType =
					Protocol::BEACON_CINEMATIC_TYPE_VILLAGE_EXIT,
				.interactionX = 335.237946f,
				.interactionZ = 590.663147f
			};
		case WorldDefId::Castle:
			return BeaconCinematicPolicy{
				.cinematicType =
					Protocol::BEACON_CINEMATIC_TYPE_CASTLE_EXIT,
				.interactionX = 338.464813f,
				.interactionZ = 417.798187f
			};
		default:
			return std::nullopt;
		}
	}

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

	bool IsDeathCountRunStartWorld(WorldDefId worldDefId) noexcept
	{
		return worldDefId == WorldDefId::Village;
	}

	bool IsDeathCountSharedWorld(WorldDefId worldDefId) noexcept
	{
		return PlayerDeathStatePolicy::IsRespawnWorld(worldDefId);
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
		.maxSessions = _config.maxSessions,
		.accountCombatStatOverride = _config.accountCombatStatOverride
	}, _framework, _startupWorldId, *this)
	, _transferBinding(_framework, _sessionSystem.Flow())
	, _partyService(*this)
	, _partyCommandPump(
		_partyCommandQueue,
		_partyService,
		_framework,
		_sessionSystem.Network())
	, _demoPartyPolicy(_partyService, *this, *this)
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
	_sessionSystem.SetPartyCommandQueue(&_partyCommandQueue);
	_partyService.Clear();
	_partyCommandQueue.Clear();
	_worldTransitionRequestIds.clear();
	_pendingClientTransitions.clear();
	_activeBeaconCinematicsByWorld.clear();
	_nextBeaconCinematicInstanceId = 1;
	_animationRegistry.Clear();
	_lastTickTime = {};

	if (!InitializeGameplayContent() ||
		!InitializeFrameworkRuntime() ||
		!InitializeDatabaseRuntime() ||
		!InitializeSessionSystem())
	{
		_sessionSystem.Shutdown();
		ShutdownDatabaseRuntime();
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
	ShutdownDatabaseRuntime();
	_framework.Shutdown();
	_initialized.store(false);
	_tickCount = 0;
	_frameIndex = 1;
	_nowSec = 0.0;
	_startupWorldId = WorldId::Invalid();
	_sessionSystem.ClearSessionState();
	_partyService.Clear();
	_partyCommandQueue.Clear();
	_worldTransitionRequestIds.clear();
	_pendingClientTransitions.clear();
	_activeBeaconCinematicsByWorld.clear();
	_nextBeaconCinematicInstanceId = 1;
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
		_sessionSystem.Flow().FindCurrentWorldId(sessionId);
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

bool ServerApp::RequestPlayerRespawn(SessionId sessionId)
{
	if (!IsInitialized() || sessionId == 0)
	{
		return false;
	}

	const WorldId worldId = _sessionSystem.Flow().FindCurrentWorldId(sessionId);
	const NetId netId = _sessionSystem.Flow().FindControlledNetId(sessionId);
	if (!worldId.IsValid() || !netId.IsValid())
	{
		return false;
	}

	WorldInstance* const world = _framework.FindWorld(worldId);
	const WorldDef* const worldDef =
		world != nullptr ? world->GetDef() : nullptr;
	if (world == nullptr ||
		worldDef == nullptr ||
		!IsDeathCountSharedWorld(worldDef->id))
	{
		return false;
	}

	const NetBindingLocation binding = _framework.FindNetBinding(netId);
	if (binding.worldId != worldId || binding.entity.IsNull())
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	PlayerControlIdentityComp* const player =
		view.GetMutableComponent<PlayerControlIdentityComp>(binding.entity);
	const CombatStatStateComp* const stats =
		view.GetComponent<CombatStatStateComp>(binding.entity);
	PlayerDeathStateComp* const deathState =
		view.GetMutableComponent<PlayerDeathStateComp>(binding.entity);
	if (player == nullptr ||
		player->ownerSessionId != sessionId ||
		stats == nullptr ||
		stats->currentHp > 0 ||
		deathState == nullptr ||
		!PlayerDeathStatePolicy::CanLatchRespawnRequest(*deathState))
	{
		return false;
	}

	deathState->respawnRequested = true;
	return true;
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
		_sessionSystem.Flow().FindCurrentWorldId(sessionId);
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

	const PartyResult partyResult =
		_demoPartyPolicy.EnsurePartyForWorldTransition(
			sessionId,
			sourceWorldId,
			_nowSec);
	if (!partyResult.Succeeded())
	{
		FWLOG_WARN(kLogCategory,
			"Demo world transition rejected: party formation failed (sid=%u, requestId=%u, partyError=%d)",
			sessionId,
			requestId,
			static_cast<int>(partyResult.error));
		return 0;
	}

	const PartyId partyId = partyResult.partyId;
	const uint64_t instanceKey =
		targetWorldDefId == WorldDefId::Plaza ? 0 : static_cast<uint64_t>(partyId);

	WorldTargetSpec target{};
	target.targetWorldDefId = targetWorldDefId;
	target.instanceKey = instanceKey;

	const SessionId leaderSessionId =
		_partyService.FindLeaderSession(partyId);
	PartyWorldEntryResult entryResult =
		_partyService.BeginWorldEntry(
			leaderSessionId,
			target,
			_nowSec,
			true);
	if (!entryResult.Succeeded())
	{
		FWLOG_WARN(kLogCategory,
			"Demo world transition rejected: party entry failed (sid=%u, requestId=%u, partyId=%llu, partyError=%d)",
			sessionId,
			requestId,
			static_cast<unsigned long long>(partyId),
			static_cast<int>(entryResult.error));
		return 0;
	}

	const TransferId transferId = _framework.RequestWorldTransfer(
		std::span<const SessionId>(
			entryResult.request.sessionIds.data(),
			entryResult.request.sessionIds.size()),
		sourceWorldId,
		targetWorldDefId,
		instanceKey,
		partyId,
		true,
		_nowSec);
	if (transferId != 0)
	{
		(void)_partyService.MarkWorldEntryEnqueued(
			partyId,
			transferId,
			_nowSec);
		_worldTransitionRequestIds[transferId][sessionId] = requestId;
	}
	else
	{
		(void)_partyService.FailWorldEntry(partyId, 0, _nowSec);
	}

	return transferId;
}

bool ServerApp::RequestBeaconCinematicStart(
	SessionId sessionId,
	uint32_t clientRequestId)
{
	if (!IsInitialized() || sessionId == 0 || clientRequestId == 0)
	{
		return false;
	}

	const WorldId sourceWorldId =
		_sessionSystem.Flow().FindCurrentWorldId(sessionId);
	WorldInstance* const sourceWorld =
		_framework.FindWorld(sourceWorldId);
	const WorldDef* const sourceWorldDef =
		sourceWorld != nullptr ? sourceWorld->GetDef() : nullptr;
	if (!sourceWorldId.IsValid() || sourceWorld == nullptr ||
		sourceWorldDef == nullptr)
	{
		return false;
	}

	const std::optional<BeaconCinematicPolicy> policy =
		ResolveBeaconCinematicPolicy(sourceWorldDef->id);
	if (!policy.has_value())
	{
		FWLOG_WARN(
			kLogCategory,
			"Beacon cinematic start rejected: unsupported world "
			"(sid=%u, worldDefId=%u)",
			sessionId,
			static_cast<uint32_t>(sourceWorldDef->id));
		return false;
	}

	const PartyId partyId = _partyService.FindPartyBySession(sessionId);
	const PartyRecord* const party = _partyService.FindParty(partyId);
	if (party == nullptr ||
		party->leaderSessionId != sessionId ||
		party->lifecycle != PartyLifecycleState::InWorld)
	{
		FWLOG_WARN(
			kLogCategory,
			"Beacon cinematic start rejected: requester is not active party "
			"leader (sid=%u, partyId=%llu)",
			sessionId,
			static_cast<unsigned long long>(partyId));
		return false;
	}

	const NetId initiatorNetId =
		_sessionSystem.Flow().FindControlledNetId(sessionId);
	const NetBindingLocation initiatorBinding =
		_framework.FindNetBinding(initiatorNetId);
	if (!initiatorNetId.IsValid() ||
		initiatorBinding.worldId != sourceWorldId ||
		initiatorBinding.entity.IsNull())
	{
		return false;
	}

	ECSView sourceView = sourceWorld->GetRuntime().MakeView();
	const WorldTransformComp* const initiatorTransform =
		sourceView.GetComponent<WorldTransformComp>(initiatorBinding.entity);
	if (initiatorTransform == nullptr)
	{
		return false;
	}

	const float deltaX =
		initiatorTransform->position.x - policy->interactionX;
	const float deltaZ =
		initiatorTransform->position.z - policy->interactionZ;
	if (deltaX * deltaX + deltaZ * deltaZ >
		kBeaconInteractionServerRadius * kBeaconInteractionServerRadius)
	{
		FWLOG_WARN(
			kLogCategory,
			"Beacon cinematic start rejected: leader is outside interaction "
			"range (sid=%u, worldId=%u, x=%.3f, z=%.3f)",
			sessionId,
			sourceWorldId.GetRaw(),
			initiatorTransform->position.x,
			initiatorTransform->position.z);
		return false;
	}

	std::vector<SessionId> partySessions;
	partySessions.reserve(party->members.size());
	for (const PartyMember& member : party->members)
	{
		if (member.sessionId == 0 ||
			member.presence != PartyMemberPresence::Online ||
			_sessionSystem.Flow().FindCurrentWorldId(member.sessionId) !=
				sourceWorldId ||
			!CanBeginWorldTransfer(member.sessionId))
		{
			FWLOG_WARN(
				kLogCategory,
				"Beacon cinematic start rejected: party member unavailable "
				"(partyId=%llu, memberSid=%u)",
				static_cast<unsigned long long>(partyId),
				member.sessionId);
			return false;
		}

		partySessions.push_back(member.sessionId);
	}

	if (partySessions.empty())
	{
		return false;
	}

	const uint64_t sourceWorldKey = sourceWorldId.GetRaw();
	if (const auto activeIt =
		_activeBeaconCinematicsByWorld.find(sourceWorldKey);
		activeIt != _activeBeaconCinematicsByWorld.end())
	{
		if (_nowSec < activeIt->second.expiresAtSec)
		{
			return false;
		}
		_activeBeaconCinematicsByWorld.erase(activeIt);
	}

	if (_nextBeaconCinematicInstanceId == 0)
	{
		_nextBeaconCinematicInstanceId = 1;
	}
	const uint64_t cinematicInstanceId =
		_nextBeaconCinematicInstanceId++;

	if (!ServerPacketStager::StageBeaconCinematicStartPacket(
		_sessionSystem.Network(),
		std::span<const SessionId>(
			partySessions.data(),
			partySessions.size()),
		cinematicInstanceId,
		clientRequestId,
		static_cast<uint64_t>(partyId),
		sourceWorldId.GetRaw(),
		static_cast<uint32_t>(policy->cinematicType),
		initiatorNetId.GetRaw()))
	{
		return false;
	}

	_activeBeaconCinematicsByWorld[sourceWorldKey] =
		ActiveBeaconCinematic{
			.partyId = partyId,
			.sourceWorldId = sourceWorldId,
			.cinematicInstanceId = cinematicInstanceId,
			.cinematicType =
				static_cast<uint32_t>(policy->cinematicType),
			.expiresAtSec = _nowSec + kBeaconCinematicStartGuardSec
		};

	FWLOG_INFO(
		kLogCategory,
		"Beacon cinematic start staged "
		"(instanceId=%llu, partyId=%llu, leaderSid=%u, worldId=%u, "
		"members=%zu)",
		static_cast<unsigned long long>(cinematicInstanceId),
		static_cast<unsigned long long>(partyId),
		sessionId,
		sourceWorldId.GetRaw(),
		partySessions.size());
	return true;
}

bool ServerApp::SubmitFinalClearPvpChoice(
	SessionId sessionId,
	uint64_t voteId,
	bool choosePvp)
{
	if (sessionId == 0 || voteId == 0)
	{
		return false;
	}

	const auto voteIt = _finalClearChoiceVotes.find(voteId);
	if (voteIt == _finalClearChoiceVotes.end())
	{
		return false;
	}

	FinalClearChoiceVote& vote = voteIt->second;
	if (!vote.eligibleSessions.contains(sessionId))
	{
		return false;
	}

	vote.choices[sessionId] = choosePvp;

	// 조작감 개선: 한 명이 PvP를 골라도 즉시 전이하지 않는다. 자격자 전원이
	// 투표를 마쳐야 판정한다(한 명이라도 PvP면 PvP, 아니면 Plaza 엔딩).
	if (vote.choices.size() >= vote.eligibleSessions.size())
	{
		return ResolveFinalClearVoteByCastChoices(
			voteId,
			static_cast<uint32_t>(Protocol::FINAL_CLEAR_REASON_ALL_DECLINED));
	}

	return true;
}

bool ServerApp::ResolveFinalClearVoteByCastChoices(
	uint64_t voteId,
	uint32_t plazaReason)
{
	const auto voteIt = _finalClearChoiceVotes.find(voteId);
	if (voteIt == _finalClearChoiceVotes.end())
	{
		return false;
	}

	// 캐스팅된 표 중 한 명이라도 PvP면 PvP로 확정, 그 외에는 Plaza(엔딩).
	for (const auto& [chooserSessionId, choosePvp] : voteIt->second.choices)
	{
		if (choosePvp)
		{
			return ResolveFinalClearChoiceVote(
				voteId,
				WorldDefId::Pvp,
				static_cast<uint32_t>(Protocol::FINAL_CLEAR_REASON_PVP_CHOSEN),
				chooserSessionId);
		}
	}

	return ResolveFinalClearChoiceVote(
		voteId,
		WorldDefId::Plaza,
		plazaReason);
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

	std::vector<NetId> pendingSnapshotExcludedNetIds;
	pendingSnapshotExcludedNetIds.reserve(_pendingClientTransitions.size());
	for (const auto& [pendingSessionId, pendingTransition] : _pendingClientTransitions)
	{
		if (pendingSessionId == sessionId ||
			pendingTransition.targetWorldId != pending.targetWorldId ||
			!pendingTransition.playerNetId.IsValid())
		{
			continue;
		}

		pendingSnapshotExcludedNetIds.push_back(pendingTransition.playerNetId);
	}

	ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
		_framework,
		_sessionSystem.Network(),
		pending.targetWorldId,
		sessionId,
		std::span<const NetId>(
			pendingSnapshotExcludedNetIds.data(),
			pendingSnapshotExcludedNetIds.size()));

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
			_sessionSystem.Flow().CollectSessionsInWorld(
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

			TitleId equippedTitleId{ InvalidTitleId };
			if (ServerReplicationSnapshot::TryGetPlazaPlayerTitleState(
				_framework,
				pending.targetWorldId,
				playerBinding.entity,
				equippedTitleId))
			{
				(void)ServerPacketStager::StageTitleReplicationPacketToSessions(
					_sessionSystem.Network(),
					std::span<const SessionId>(
						otherReadySessionIds.data(),
						otherReadySessionIds.size()),
					pending.playerNetId,
					equippedTitleId);
			}
		}
	}

	_pendingClientTransitions.erase(it);
	return true;
}

void ServerApp::OnSessionDisconnected(SessionId sessionId) noexcept
{
	_pendingClientTransitions.erase(sessionId);
	_partyCommandQueue.Submit(PartyCommand{
		.kind = PartyCommandKind::MarkMemberOffline,
		.actorSessionId = sessionId,
		.submittedAtSec = _nowSec
	});
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
	_sessionSystem.SetDatabaseBackend(_databaseBackend.get());
	return _sessionSystem.Initialize();
}

bool ServerApp::InitializeDatabaseRuntime()
{
	_sessionSystem.SetDatabaseBackend(nullptr);

	if (!_config.database.enabled)
	{
		FWLOG_INFO(kLogCategory, "Database backend disabled");
		return true;
	}

	if (_databaseBackend != nullptr)
	{
		return true;
	}

	auto databaseBackend =
		std::make_unique<ODBCDatabaseBackend>(
			_config.database,
			_framework.GetIOSink());

	_framework.RegisterIOBackend(databaseBackend.get());
	if (!databaseBackend->Start())
	{
		_framework.UnregisterIOBackend(databaseBackend.get());
		FWLOG_FATAL(kLogCategory, "Database backend start failed");
		return false;
	}

	_databaseBackend = std::move(databaseBackend);

	// DB가 활성화된 경우에만 파티 영속화 gateway를 붙이고 복구를 시작한다.
	_partyDbResultQueue.Clear();
	_partyPersistGateway = std::make_unique<PartyPersistGateway>(
		*_databaseBackend,
		_partyDbResultQueue,
		_partyService);
	_partyCommandPump.SetPersistGateway(_partyPersistGateway.get());
	_partyPersistGateway->SubmitStartupLoad();

	return true;
}

void ServerApp::ShutdownDatabaseRuntime() noexcept
{
	_sessionSystem.SetDatabaseBackend(nullptr);

	_partyCommandPump.SetPersistGateway(nullptr);
	_partyPersistGateway.reset();
	_partyDbResultQueue.Clear();

	if (_databaseBackend == nullptr)
	{
		return;
	}

	_databaseBackend->Stop();
	_framework.UnregisterIOBackend(_databaseBackend.get());
	_databaseBackend.reset();
}

bool ServerApp::InitializeGameplayContent()
{
	_animationRegistry.Clear();

	const GameplayContentCatalogRoots gameplayContentRoots
	{
		.attributeRoot = ServerPathResolver::GetDefaultAttributeDefRoot(),
		.tagRoot = ServerPathResolver::GetDefaultGameplayTagDefRoot(),
		.effectRoot = ServerPathResolver::GetDefaultGameplayEffectDefRoot(),
		.projectileRoot = ServerPathResolver::GetDefaultProjectileDefRoot(),
		.areaHitRoot = ServerPathResolver::GetDefaultAreaHitDefRoot(),
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
		.characterRoot  = ServerPathResolver::GetDefaultCharacterDefRoot(),
		.aiBehaviorRoot = ServerPathResolver::GetDefaultAIBehaviorDefRoot(),
		.spawnSetRoot   = ServerPathResolver::GetDefaultSpawnSetDefRoot(),
		.titleRoot      = ServerPathResolver::GetDefaultTitleDefRoot()
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
	_sessionSystem.StageTimeSyncPackets(_frameIndex > 0 ? _frameIndex - 1 : 0);

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
		_sessionSystem.Flow(),
		std::span<const SessionId>(
			pendingTransitionSessions.data(),
			pendingTransitionSessions.size()));
	FlushOutbound();

	++_tickCount;
}

void ServerApp::RunWorldFrames(double dtSec)
{
	_nowSec += dtSec;

	_partyCommandPump.Pump(_nowSec);

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
		_sessionSystem.Flow(),
		transferEvents))
	{
		FWLOG_ERROR(kLogCategory, "World transfer commit failed");
		Stop();
		return;
	}

	ApplyPartyWorldTransferEvents(transferEvents);

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

	if (!ApplyPartyDeathCountEvents(frameResult))
	{
		FWLOG_ERROR(kLogCategory, "Party death count event apply failed");
		Stop();
		return;
	}

	if (!ApplyFinalBossDefeatedEvents(frameResult))
	{
		FWLOG_ERROR(kLogCategory, "Final boss defeated event apply failed");
		Stop();
		return;
	}

	TickFinalClearChoiceVotes();
	TickPendingEndingTransfers();

	if (!ProcessPvpRoundEndConditions())
	{
		FWLOG_ERROR(kLogCategory, "PvP round end processing failed");
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

void ServerApp::ApplyPartyWorldTransferEvents(
	const WorldTransferEventBatch& transferEvents)
{
	for (const WorldTransferFailedEvent& failed : transferEvents.failed)
	{
		if (failed.partyId != 0)
		{
			(void)_partyService.FailWorldEntry(
				failed.partyId,
				failed.transferId,
				_nowSec);

		if (_partyPersistGateway != nullptr)
		{
			_partyPersistGateway->PersistCurrentState(failed.partyId, _nowSec);
		}
		}
	}

	for (const WorldTransferCompletedEvent& completed : transferEvents.completed)
	{
		_activeBeaconCinematicsByWorld.erase(
			completed.sourceWorldId.GetRaw());

		if (completed.partyId != 0)
		{
			const PartyResult completeResult =
				_partyService.CompleteWorldEntry(
				completed.partyId,
				completed.transferId,
				completed.targetWorldId,
				_nowSec);
			if (!completeResult.Succeeded())
			{
				FWLOG_WARN(kLogCategory,
					"Party world entry complete failed (partyId=%llu, transferId=%u, targetWorldId=%u, error=%u)",
					static_cast<unsigned long long>(completed.partyId),
					completed.transferId,
					completed.targetWorldId.GetRaw(),
					static_cast<uint32_t>(completeResult.error));
				continue;
			}

			if (_partyPersistGateway != nullptr)
			{
				_partyPersistGateway->PersistCurrentState(completed.partyId, _nowSec);
			}

			const WorldInstance* const targetWorld =
				_framework.FindWorld(completed.targetWorldId);
			const WorldDef* const targetDef =
				targetWorld != nullptr ? targetWorld->GetDef() : nullptr;
			const WorldInstance* const sourceWorld =
				_framework.FindWorld(completed.sourceWorldId);
			const WorldDef* const sourceDef =
				sourceWorld != nullptr ? sourceWorld->GetDef() : nullptr;
			if (sourceDef != nullptr && sourceDef->id == WorldDefId::Pvp)
			{
				_activePvpRounds.erase(completed.sourceWorldId.GetRaw());
			}
			if (targetDef != nullptr &&
				targetDef->id == WorldDefId::Pvp &&
				completed.partyId != 0)
			{
				_activePvpRounds[completed.targetWorldId.GetRaw()] =
					ActivePvpRound{
						.partyId = completed.partyId,
						.worldId = completed.targetWorldId,
						.startedAtSec = _nowSec,
						.ending = false
					};
			}
			if (targetDef != nullptr &&
				IsDeathCountRunStartWorld(targetDef->id))
			{
				const PartyDeathCountResult deathCountResult =
					_partyService.InitializeDeathCountForRun(
						completed.partyId,
						_nowSec);
				if (!deathCountResult.Succeeded())
				{
					FWLOG_WARN(kLogCategory,
						"Party death count init failed (partyId=%llu, error=%u)",
						static_cast<unsigned long long>(completed.partyId),
						static_cast<uint32_t>(deathCountResult.error));
				}
				else if (!StagePartyDeathCountSync(
					completed.partyId,
					deathCountResult.deathCount))
				{
					FWLOG_WARN(kLogCategory,
						"Party death count sync stage failed (partyId=%llu, remaining=%u, initial=%u)",
						static_cast<unsigned long long>(completed.partyId),
						deathCountResult.deathCount.remainingCount,
						deathCountResult.deathCount.initialCount);
				}
			}
			else if (targetDef != nullptr &&
				IsDeathCountSharedWorld(targetDef->id))
			{
				const PartyDeathCountState deathCount =
					_partyService.GetDeathCountSnapshot(completed.partyId);
				if (deathCount.initialized &&
					!StagePartyDeathCountSync(completed.partyId, deathCount))
				{
					FWLOG_WARN(kLogCategory,
						"Party death count resync stage failed (partyId=%llu, remaining=%u, initial=%u)",
						static_cast<unsigned long long>(completed.partyId),
						deathCount.remainingCount,
						deathCount.initialCount);
				}
			}
		}
	}
}

bool ServerApp::ApplyPartyDeathCountEvents(
	const FrameworkRuntime::FrameResult& frameResult)
{
	std::unordered_map<PartyId, WorldId> exhaustedParties;

	for (const auto& deathEvent : frameResult.events.playerDeathCounts)
	{
		if (deathEvent.sessionId == 0)
		{
			continue;
		}

		const WorldInstance* const world =
			_framework.FindWorld(deathEvent.worldId);
		const WorldDef* const worldDef =
			world != nullptr ? world->GetDef() : nullptr;
		if (worldDef == nullptr ||
			!IsDeathCountSharedWorld(worldDef->id))
		{
			continue;
		}

		const PartyId partyId =
			_partyService.FindPartyBySession(deathEvent.sessionId);
		if (partyId == 0)
		{
			(void)ApplyPlayerDeathCountDecision(deathEvent, false, 0);
			continue;
		}

		const PartyDeathCountResult result =
			_partyService.ConsumeDeathCount(
				partyId,
				deathEvent.sessionId,
				_nowSec);
		if (!result.Succeeded())
		{
			FWLOG_WARN(kLogCategory,
				"Party death count consume failed (partyId=%llu, sid=%u, worldId=%u, error=%u)",
				static_cast<unsigned long long>(partyId),
				deathEvent.sessionId,
				deathEvent.worldId.GetRaw(),
				static_cast<uint32_t>(result.error));
			(void)ApplyPlayerDeathCountDecision(deathEvent, false, 0);
			continue;
		}

		if (!ApplyPlayerDeathCountDecision(
				deathEvent,
				result.consumed,
				result.deathCount.revision))
		{
			FWLOG_WARN(kLogCategory,
				"Player death count decision apply failed (sid=%u, worldId=%u, entity=%d, canRespawn=%u)",
				deathEvent.sessionId,
				deathEvent.worldId.GetRaw(),
				deathEvent.entity.id,
				result.consumed ? 1u : 0u);
		}

		if (!StagePartyDeathCountSync(partyId, result.deathCount))
		{
			FWLOG_WARN(kLogCategory,
				"Party death count sync stage failed (partyId=%llu, sid=%u, remaining=%u, initial=%u)",
				static_cast<unsigned long long>(partyId),
				deathEvent.sessionId,
				result.deathCount.remainingCount,
				result.deathCount.initialCount);
		}

		if (result.consumed)
		{
			FWLOG_INFO(kLogCategory,
				"Party death count consumed (partyId=%llu, sid=%u, remaining=%u, initial=%u, revision=%llu)",
				static_cast<unsigned long long>(partyId),
				deathEvent.sessionId,
				result.deathCount.remainingCount,
				result.deathCount.initialCount,
				static_cast<unsigned long long>(result.deathCount.revision));
		}

		if (result.deathCount.exhausted)
		{
			exhaustedParties[partyId] = deathEvent.worldId;
			FWLOG_INFO(kLogCategory,
				"Party death count exhausted (partyId=%llu, sid=%u, revision=%llu)",
				static_cast<unsigned long long>(partyId),
				deathEvent.sessionId,
				static_cast<unsigned long long>(result.deathCount.revision));
		}
	}

	for (const auto& [partyId, sourceWorldId] : exhaustedParties)
	{
		if (!IsPartyWipedInWorld(partyId, sourceWorldId))
		{
			continue;
		}

		if (!ForcePartyGroupTransfer(
				partyId,
				sourceWorldId,
				WorldDefId::Plaza))
		{
			FWLOG_WARN(kLogCategory,
				"Exhausted party wipe transfer failed (partyId=%llu, sourceWorldId=%u)",
				static_cast<unsigned long long>(partyId),
				sourceWorldId.GetRaw());
			continue;
		}

		FWLOG_INFO(kLogCategory,
			"Exhausted party wiped; returning to Plaza (partyId=%llu, sourceWorldId=%u)",
			static_cast<unsigned long long>(partyId),
			sourceWorldId.GetRaw());
	}

	return true;
}

bool ServerApp::IsPartyWipedInWorld(
	PartyId partyId,
	WorldId sourceWorldId)
{
	const PartyRecord* const party = _partyService.FindParty(partyId);
	const PartyDeathCountState deathCount =
		_partyService.GetDeathCountSnapshot(partyId);
	WorldInstance* const world = _framework.FindWorld(sourceWorldId);
	const WorldDef* const worldDef =
		world != nullptr ? world->GetDef() : nullptr;
	if (party == nullptr ||
		!deathCount.initialized ||
		!deathCount.exhausted ||
		deathCount.remainingCount != 0 ||
		world == nullptr ||
		worldDef == nullptr ||
		!IsDeathCountSharedWorld(worldDef->id))
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	uint32_t partyMemberCount = 0;
	uint32_t aliveMemberCount = 0;
	for (const PartyMember& member : party->members)
	{
		if (member.sessionId == 0 ||
			member.presence != PartyMemberPresence::Online)
		{
			continue;
		}

		if (_sessionSystem.Flow().FindCurrentWorldId(member.sessionId) !=
				sourceWorldId ||
			!CanBeginWorldTransfer(member.sessionId))
		{
			return false;
		}

		const NetId netId =
			_sessionSystem.Flow().FindControlledNetId(member.sessionId);
		if (!netId.IsValid())
		{
			return false;
		}

		const NetBindingLocation binding = _framework.FindNetBinding(netId);
		if (binding.worldId != sourceWorldId ||
			binding.entity.IsNull())
		{
			return false;
		}

		const PlayerControlIdentityComp* const player =
			view.GetComponent<PlayerControlIdentityComp>(binding.entity);
		const CombatStatStateComp* const stats =
			view.GetComponent<CombatStatStateComp>(binding.entity);
		if (player == nullptr ||
			player->ownerSessionId != member.sessionId ||
			stats == nullptr ||
			view.HasComponent<PendingWorldTransferTag>(binding.entity) ||
			view.HasComponent<PendingDespawnTag>(binding.entity))
		{
			return false;
		}

		++partyMemberCount;
		if (stats->currentHp > 0)
		{
			++aliveMemberCount;
		}
	}

	return PlayerDeathStatePolicy::ShouldReturnExhaustedPartyToPlaza(
		deathCount.exhausted,
		partyMemberCount,
		aliveMemberCount);
}

bool ServerApp::ApplyFinalBossDefeatedEvents(
	const FrameworkRuntime::FrameResult& frameResult)
{
	for (const auto& event : frameResult.events.finalBossDefeats)
	{
		if (!event.worldId.IsValid())
		{
			continue;
		}

		const WorldInstance* const world =
			_framework.FindWorld(event.worldId);
		const WorldDef* const worldDef =
			world != nullptr ? world->GetDef() : nullptr;
		if (worldDef == nullptr || worldDef->id != WorldDefId::Final)
		{
			continue;
		}

		if (!StartFinalClearChoiceVote(event.worldId))
		{
			FWLOG_WARN(kLogCategory,
				"Final clear choice start failed (worldId=%u, bossNetId=%u)",
				event.worldId.GetRaw(),
				event.bossNetId.GetRaw());
		}
	}

	return true;
}

void ServerApp::TickFinalClearChoiceVotes()
{
	std::vector<uint64_t> expiredVoteIds;
	for (const auto& [voteId, vote] : _finalClearChoiceVotes)
	{
		if (_nowSec >= vote.deadlineSec)
		{
			expiredVoteIds.push_back(voteId);
		}
	}

	for (uint64_t voteId : expiredVoteIds)
	{
		// 제한 시간 만료 → 미투표자는 거절로 간주하고, 그때까지 캐스팅된 표
		// 기준으로 판정한다(한 명이라도 PvP를 골랐다면 PvP로 간다).
		(void)ResolveFinalClearVoteByCastChoices(
			voteId,
			static_cast<uint32_t>(Protocol::FINAL_CLEAR_REASON_TIMEOUT));
	}
}

bool ServerApp::StartFinalClearChoiceVote(WorldId sourceWorldId)
{
	if (!sourceWorldId.IsValid())
	{
		return false;
	}

	if (_finalClearChoiceVoteByWorld.contains(sourceWorldId.GetRaw()))
	{
		return true;
	}

	std::vector<SessionId> worldSessionIds;
	_sessionSystem.Flow().CollectSessionsInWorld(sourceWorldId, worldSessionIds);
	if (worldSessionIds.empty())
	{
		return true;
	}

	const PartyId partyId =
		_partyService.FindPartyBySession(worldSessionIds.front());
	const PartyRecord* const party = _partyService.FindParty(partyId);
	if (party == nullptr)
	{
		return true;
	}

	if (party->members.size() < 2)
	{
		// 파티원 1명: 투표 없이도 바로 강제 전이하지 않고 엔딩 연출 후 Plaza로.
		StartFinalEndingForParty(partyId, sourceWorldId);
		return true;
	}

	FinalClearChoiceVote vote{};
	vote.voteId = _nextFinalClearChoiceVoteId++;
	vote.partyId = partyId;
	vote.sourceWorldId = sourceWorldId;
	vote.deadlineSec = _nowSec + kFinalClearChoiceTimeoutSec;

	for (const PartyMember& member : party->members)
	{
		if (member.sessionId == 0 ||
			member.presence != PartyMemberPresence::Online ||
			_sessionSystem.Flow().FindCurrentWorldId(member.sessionId) !=
				sourceWorldId ||
			!CanBeginWorldTransfer(member.sessionId))
		{
			continue;
		}

		vote.eligibleSessions.insert(member.sessionId);
	}

	if (vote.eligibleSessions.size() < 2)
	{
		// 자격자 1명 이하: 투표 없이 엔딩 연출 후 Plaza로.
		StartFinalEndingForParty(partyId, sourceWorldId);
		return true;
	}

	const uint64_t voteId = vote.voteId;
	const double deadlineSec = vote.deadlineSec;

	std::vector<SessionId> beginSessions;
	beginSessions.reserve(vote.eligibleSessions.size());
	for (const SessionId eligibleSessionId : vote.eligibleSessions)
	{
		beginSessions.push_back(eligibleSessionId);
	}
	const uint32_t eligibleCount =
		static_cast<uint32_t>(beginSessions.size());

	_finalClearChoiceVoteByWorld[sourceWorldId.GetRaw()] = voteId;
	_finalClearChoiceVotes.emplace(voteId, std::move(vote));

	(void)ServerPacketStager::StageFinalClearChoiceBeginPacket(
		_sessionSystem.Network(),
		std::span<const SessionId>(beginSessions.data(), beginSessions.size()),
		voteId,
		static_cast<uint64_t>(partyId),
		sourceWorldId.GetRaw(),
		static_cast<uint32_t>(kFinalClearChoiceTimeoutSec * 1000.0),
		static_cast<uint64_t>(deadlineSec * 1000.0),
		eligibleCount);
	return true;
}

bool ServerApp::ResolveFinalClearChoiceVote(
	uint64_t voteId,
	WorldDefId targetWorldDefId,
	uint32_t reason,
	SessionId pvpChooserSessionId)
{
	const auto voteIt = _finalClearChoiceVotes.find(voteId);
	if (voteIt == _finalClearChoiceVotes.end())
	{
		return false;
	}

	const PartyId partyId = voteIt->second.partyId;
	const WorldId sourceWorldId = voteIt->second.sourceWorldId;

	// 결과 통지는 투표에 참여한 자격자 전원에게 보낸다(투표 엔트리가 지워지기 전에).
	std::vector<SessionId> resultSessions;
	resultSessions.reserve(voteIt->second.eligibleSessions.size());
	for (const SessionId eligibleSessionId : voteIt->second.eligibleSessions)
	{
		resultSessions.push_back(eligibleSessionId);
	}

	const bool toPvp = targetWorldDefId == WorldDefId::Pvp;
	const uint32_t outcome = static_cast<uint32_t>(
		toPvp
			? Protocol::FINAL_CLEAR_OUTCOME_PVP
			: Protocol::FINAL_CLEAR_OUTCOME_ENDING);
	const uint64_t pvpChooserNetId =
		(toPvp && pvpChooserSessionId != 0)
			? FindControlledNetId(pvpChooserSessionId).GetRaw()
			: 0;

	_finalClearChoiceVoteByWorld.erase(sourceWorldId.GetRaw());
	_finalClearChoiceVotes.erase(voteIt);

	(void)ServerPacketStager::StageFinalClearChoiceResultPacket(
		_sessionSystem.Network(),
		std::span<const SessionId>(resultSessions.data(), resultSessions.size()),
		voteId,
		outcome,
		reason,
		static_cast<uint32_t>(targetWorldDefId),
		pvpChooserNetId);

	if (toPvp)
	{
		// PvP는 연출 없이 즉시 전이.
		return RequestPartyWorldTransfer(partyId, WorldDefId::Pvp);
	}

	// 엔딩(Plaza): 즉시 전이하지 않고, 클라 엔딩 연출 완료(또는 타임아웃) 후 전이.
	BeginEndingThenTransfer(
		partyId,
		sourceWorldId,
		WorldDefId::Plaza,
		std::span<const SessionId>(resultSessions.data(), resultSessions.size()));
	return true;
}

bool ServerApp::RequestPartyWorldTransfer(
	PartyId partyId,
	WorldDefId targetWorldDefId)
{
	if (partyId == 0 || targetWorldDefId == WorldDefId::None)
	{
		return false;
	}

	const SessionId leaderSessionId =
		_partyService.FindLeaderSession(partyId);
	if (leaderSessionId == 0)
	{
		return false;
	}

	WorldTargetSpec target{};
	target.targetWorldDefId = targetWorldDefId;
	target.instanceKey =
		targetWorldDefId == WorldDefId::Pvp
		? static_cast<uint64_t>(partyId)
		: 0;

	PartyWorldEntryResult entryResult =
		_partyService.BeginWorldEntry(
			leaderSessionId,
			target,
			_nowSec,
			true);
	if (!entryResult.Succeeded())
	{
		FWLOG_WARN(kLogCategory,
			"Party world transfer begin failed (partyId=%llu, target=%u, error=%u)",
			static_cast<unsigned long long>(partyId),
			static_cast<uint32_t>(targetWorldDefId),
			static_cast<uint32_t>(entryResult.error));
		return false;
	}

	const TransferId transferId = _framework.RequestWorldTransfer(
		std::span<const SessionId>(
			entryResult.request.sessionIds.data(),
			entryResult.request.sessionIds.size()),
		entryResult.request.sourceWorldId,
		targetWorldDefId,
		target.instanceKey,
		partyId,
		true,
		_nowSec);
	if (transferId == 0)
	{
		(void)_partyService.FailWorldEntry(partyId, 0, _nowSec);
		return false;
	}

	(void)_partyService.MarkWorldEntryEnqueued(
		partyId,
		transferId,
		_nowSec);
	return true;
}

bool ServerApp::ForcePartyGroupTransfer(
	PartyId partyId,
	WorldId sourceWorldId,
	WorldDefId targetWorldDefId)
{
	if (partyId == 0 ||
		!sourceWorldId.IsValid() ||
		targetWorldDefId == WorldDefId::None)
	{
		return false;
	}

	WorldTargetSpec target{};
	target.targetWorldDefId = targetWorldDefId;
	target.instanceKey =
		targetWorldDefId == WorldDefId::Pvp
		? static_cast<uint64_t>(partyId)
		: 0;

	// 리더 비의존: 소스월드에 실재하는 전송 가능 멤버(사망자 포함)만 모은다.
	PartyWorldEntryResult entryResult =
		_partyService.BeginForcedWorldEntry(
			partyId,
			sourceWorldId,
			target,
			_nowSec,
			true);
	if (!entryResult.Succeeded())
	{
		FWLOG_WARN(kLogCategory,
			"Forced party group transfer begin failed (partyId=%llu, target=%u, error=%u)",
			static_cast<unsigned long long>(partyId),
			static_cast<uint32_t>(targetWorldDefId),
			static_cast<uint32_t>(entryResult.error));
		return false;
	}

	const TransferId transferId = _framework.RequestWorldTransfer(
		std::span<const SessionId>(
			entryResult.request.sessionIds.data(),
			entryResult.request.sessionIds.size()),
		entryResult.request.sourceWorldId,
		targetWorldDefId,
		target.instanceKey,
		partyId,
		true,
		_nowSec);
	if (transferId == 0)
	{
		(void)_partyService.FailWorldEntry(partyId, 0, _nowSec);
		return false;
	}

	(void)_partyService.MarkWorldEntryEnqueued(partyId, transferId, _nowSec);
	return true;
}

void ServerApp::BeginEndingThenTransfer(
	PartyId partyId,
	WorldId sourceWorldId,
	WorldDefId targetWorldDefId,
	std::span<const SessionId> members)
{
	if (partyId == 0)
	{
		return;
	}

	PendingEndingTransfer pending{};
	pending.partyId = partyId;
	pending.sourceWorldId = sourceWorldId;
	pending.targetWorldDefId = targetWorldDefId;
	pending.deadlineSec = _nowSec + kEndingCinematicTimeoutSec;
	for (const SessionId sessionId : members)
	{
		if (sessionId != 0)
		{
			pending.awaitingAck.insert(sessionId);
		}
	}

	// 기다릴 대상이 없으면(전원 이탈 등) 즉시 전이한다.
	if (pending.awaitingAck.empty())
	{
		(void)ForcePartyGroupTransfer(partyId, sourceWorldId, targetWorldDefId);
		return;
	}

	_pendingEndingTransfers[static_cast<uint64_t>(partyId)] = std::move(pending);
}

void ServerApp::StartFinalEndingForParty(
	PartyId partyId,
	WorldId sourceWorldId)
{
	const PartyRecord* const party = _partyService.FindParty(partyId);
	if (party == nullptr)
	{
		return;
	}

	std::vector<SessionId> members;
	members.reserve(party->members.size());
	for (const PartyMember& member : party->members)
	{
		if (member.sessionId == 0 ||
			member.presence != PartyMemberPresence::Online ||
			_sessionSystem.Flow().FindCurrentWorldId(member.sessionId) !=
				sourceWorldId)
		{
			continue;
		}
		members.push_back(member.sessionId);
	}

	if (members.empty())
	{
		// 전이 대상이 없으면 연출 생략하고 강제 전이만 시도.
		(void)ForcePartyGroupTransfer(partyId, sourceWorldId, WorldDefId::Plaza);
		return;
	}

	// 투표 없는 엔딩: voteId=0 으로 ENDING 결과를 보내 클라 연출을 트리거한다.
	(void)ServerPacketStager::StageFinalClearChoiceResultPacket(
		_sessionSystem.Network(),
		std::span<const SessionId>(members.data(), members.size()),
		0,
		static_cast<uint32_t>(Protocol::FINAL_CLEAR_OUTCOME_ENDING),
		static_cast<uint32_t>(Protocol::FINAL_CLEAR_REASON_ALL_DECLINED),
		static_cast<uint32_t>(WorldDefId::Plaza),
		0);

	BeginEndingThenTransfer(
		partyId,
		sourceWorldId,
		WorldDefId::Plaza,
		std::span<const SessionId>(members.data(), members.size()));
}

bool ServerApp::SubmitFinalEndingCinematicDone(
	SessionId sessionId,
	uint32_t context)
{
	(void)context;  // 검증/로깅용. 현재는 파티 단위 대기로만 처리.
	if (sessionId == 0)
	{
		return false;
	}

	const PartyId partyId = _partyService.FindPartyBySession(sessionId);
	const auto it = _pendingEndingTransfers.find(static_cast<uint64_t>(partyId));
	if (it == _pendingEndingTransfers.end())
	{
		return false;
	}

	it->second.awaitingAck.erase(sessionId);
	if (!it->second.awaitingAck.empty())
	{
		return true;
	}

	const WorldId sourceWorldId = it->second.sourceWorldId;
	const WorldDefId targetWorldDefId = it->second.targetWorldDefId;
	_pendingEndingTransfers.erase(it);
	(void)ForcePartyGroupTransfer(partyId, sourceWorldId, targetWorldDefId);
	return true;
}

void ServerApp::TickPendingEndingTransfers()
{
	std::vector<uint64_t> expiredPartyIds;
	for (const auto& [partyIdRaw, pending] : _pendingEndingTransfers)
	{
		if (_nowSec >= pending.deadlineSec)
		{
			expiredPartyIds.push_back(partyIdRaw);
		}
	}

	for (uint64_t partyIdRaw : expiredPartyIds)
	{
		const auto it = _pendingEndingTransfers.find(partyIdRaw);
		if (it == _pendingEndingTransfers.end())
		{
			continue;
		}

		const WorldId sourceWorldId = it->second.sourceWorldId;
		const WorldDefId targetWorldDefId = it->second.targetWorldDefId;
		_pendingEndingTransfers.erase(it);
		// 타임아웃: 일부가 연출 완료를 보고하지 않아도 강제로 전이한다.
		(void)ForcePartyGroupTransfer(
			static_cast<PartyId>(partyIdRaw),
			sourceWorldId,
			targetWorldDefId);
	}
}

bool ServerApp::ProcessPvpRoundEndConditions()
{
	for (auto& [worldIdRaw, round] : _activePvpRounds)
	{
		(void)worldIdRaw;
		if (round.ending)
		{
			continue;
		}

		WorldInstance* const world = _framework.FindWorld(round.worldId);
		const WorldDef* const worldDef =
			world != nullptr ? world->GetDef() : nullptr;
		if (world == nullptr || worldDef == nullptr)
		{
			round.ending = true;
			continue;
		}

		if (worldDef->id != WorldDefId::Pvp)
		{
			round.ending = true;
			continue;
		}

		ECSView view = world->GetRuntime().MakeView();
		uint32_t playerCount = 0;
		uint32_t aliveCount = 0;
		SessionId winnerSessionId = 0;
		for (auto [entity, player, stats] :
			view.View<
				PlayerControlIdentityComp,
				CombatStatStateComp>())
		{
			if (player.ownerSessionId == 0 ||
				view.HasComponent<PendingWorldTransferTag>(entity) ||
				view.HasComponent<PendingDespawnTag>(entity))
			{
				continue;
			}

			++playerCount;
			if (stats.currentHp > 0)
			{
				++aliveCount;
				winnerSessionId = player.ownerSessionId;
			}
		}

		// 라스트맨: 생존자가 1명 이하가 되면 라운드 종료. 사망자는 despawn되지
		// 않고 그대로 남아있으므로(ResolveDeathAndDespawnSystem), 결과 통지/전이
		// 대상에 함께 포함된다.
		if (playerCount > 0 && aliveCount <= 1)
		{
			round.ending = true;

			const uint64_t winnerNetId =
				winnerSessionId != 0
				? FindControlledNetId(winnerSessionId).GetRaw()
				: 0;

			std::vector<SessionId> pvpSessions;
			_sessionSystem.Flow().CollectSessionsInWorld(
				round.worldId,
				pvpSessions);

			// 결과(라스트맨) 통지 → 클라 페이드/엔딩 연출 트리거.
			(void)ServerPacketStager::StagePvpRoundResultPacket(
				_sessionSystem.Network(),
				std::span<const SessionId>(pvpSessions.data(), pvpSessions.size()),
				winnerNetId);

			// 연출 완료(또는 타임아웃) 후 생존자/사망자 전원을 Plaza로 전이.
			BeginEndingThenTransfer(
				round.partyId,
				round.worldId,
				WorldDefId::Plaza,
				std::span<const SessionId>(pvpSessions.data(), pvpSessions.size()));
		}
	}

	return true;
}

bool ServerApp::StagePartyDeathCountSync(
	PartyId partyId,
	const PartyDeathCountState& deathCount)
{
	const PartyRecord* const party = _partyService.FindParty(partyId);
	if (party == nullptr || !deathCount.initialized)
	{
		return false;
	}

	std::vector<SessionId> sessionIds;
	sessionIds.reserve(party->members.size());
	for (const PartyMember& member : party->members)
	{
		if (member.sessionId == 0 ||
			member.presence != PartyMemberPresence::Online ||
			!IsPartyEligible(member.sessionId))
		{
			continue;
		}

		sessionIds.push_back(member.sessionId);
	}

	return ServerPacketStager::StageTeamDeathCountPacketToSessions(
		_sessionSystem.Network(),
		std::span<const SessionId>(sessionIds.data(), sessionIds.size()),
		deathCount);
}

bool ServerApp::ApplyPlayerDeathCountDecision(
	const FrameworkRuntime::FrameResult::PlayerDeathCountEvent& deathEvent,
	bool canRespawn,
	uint64_t deathCountRevision)
{
	if (!deathEvent.worldId.IsValid() ||
		deathEvent.entity.IsNull() ||
		deathEvent.sessionId == 0)
	{
		return false;
	}

	WorldInstance* const world = _framework.FindWorld(deathEvent.worldId);
	const WorldDef* const worldDef =
		world != nullptr ? world->GetDef() : nullptr;
	if (world == nullptr ||
		worldDef == nullptr ||
		!IsDeathCountSharedWorld(worldDef->id))
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	PlayerControlIdentityComp* const player =
		view.GetMutableComponent<PlayerControlIdentityComp>(deathEvent.entity);
	PlayerDeathStateComp* const deathState =
		view.GetMutableComponent<PlayerDeathStateComp>(deathEvent.entity);
	if (player == nullptr ||
		player->ownerSessionId != deathEvent.sessionId ||
		deathState == nullptr ||
		deathState->state != PlayerDeathState::WaitingForDeathCount)
	{
		return false;
	}

	return PlayerDeathStatePolicy::ApplyDeathCountDecision(
		*deathState,
		canRespawn,
		deathCountRevision);
}

bool ServerApp::IsPartyEligible(SessionId sessionId) const
{
	const SessionFlow* const flow =
		_sessionSystem.Flow().FindFlow(sessionId);
	return flow != nullptr &&
		flow->HasBinding() &&
		flow->stateId != SessionStateId::Closing;
}

uint64_t ServerApp::FindAccountId(SessionId sessionId) const
{
	const SessionFlow* const flow =
		_sessionSystem.Flow().FindFlow(sessionId);
	return flow != nullptr ? flow->accountId : 0;
}

CharacterId ServerApp::FindSelectedCharacterId(SessionId sessionId) const
{
	const SessionFlow* const flow =
		_sessionSystem.Flow().FindFlow(sessionId);
	return flow != nullptr ? flow->selectedCharacterId : CharacterId::None;
}

NetId ServerApp::FindControlledNetId(SessionId sessionId) const
{
	return _sessionSystem.Flow().FindControlledNetId(sessionId);
}

WorldId ServerApp::FindCurrentWorldId(SessionId sessionId) const
{
	return _sessionSystem.Flow().FindCurrentWorldId(sessionId);
}

void ServerApp::CollectSessionsInWorld(
	WorldId worldId,
	std::vector<SessionId>& outSessionIds) const
{
	_sessionSystem.Flow().CollectSessionsInWorld(worldId, outSessionIds);
}

bool ServerApp::CanBeginWorldTransfer(SessionId sessionId) const
{
	const SessionFlow* const flow =
		_sessionSystem.Flow().FindFlow(sessionId);
	return flow != nullptr &&
		flow->HasBinding() &&
		!flow->HasPendingTransfer() &&
		!_pendingClientTransitions.contains(sessionId);
}

bool ServerApp::IsClientTransitionPending(SessionId sessionId) const
{
	return _pendingClientTransitions.contains(sessionId);
}
