#include "pch.h"
#include "ServerSessionSystem.h"

#include <algorithm>

#include "DynamicTaskTypes.h"
#include "ExecutionSourceTypes.h"
#include "FrameworkLog.h"
#include "FrameworkRuntime.h"
#include "IWorldTransitionRequestSink.h"
#include "PacketHandlers.h"
#include "ServerReplicationSnapshot.h"
#include "WorldDef.h"
#include "WorldInstance.h"

ServerSessionSystem::ServerSessionSystem(
	Config config,
	FrameworkRuntime& framework,
	const WorldId& startupWorldId,
	IWorldTransitionRequestSink& worldTransitionSink)
	: _config(config)
	, _framework(framework)
	, _startupWorldId(startupWorldId)
	, _worldTransitionSink(worldTransitionSink)
	, _network(NetworkRuntime::Config{
		.workerThreadCount = _config.networkThreadCount,
		.listenPort        = _config.listenPort,
		.maxSessions       = _config.maxSessions
	})
	, _characterDataService(CharacterDataService::Dependencies{
		.framework      = &_framework,
		.startupWorldId = &_startupWorldId
	})
	, _characterSpawnService(CharacterSpawnService::Dependencies{
		.framework = &_framework,
		.accountCombatStatOverride = _config.accountCombatStatOverride
	})
	, _sessionFlowController(SessionFlowDependencies{
		.network            = &_network,
		.framework          = &_framework,
		.characterData      = &_characterDataService,
		.characterSpawn     = &_characterSpawnService,
		.worldTransitionSink = &_worldTransitionSink
	})
{
}

bool ServerSessionSystem::TryResolvePlayerEntityLocation(
	const FrameworkRuntime& framework,
	const SessionFlow& flow,
	WorldId& outWorldId,
	Entity& outEntity) noexcept
{
	outWorldId = flow.currentWorldId.IsValid()
		? flow.currentWorldId
		: flow.playerWorldId;
	outEntity = flow.playerEntity;

	if ((outEntity.IsNull() || !outWorldId.IsValid()) &&
		flow.controlledNetId.IsValid())
	{
		const NetBindingLocation binding =
			framework.FindNetBinding(flow.controlledNetId);
		if (binding.IsValid())
		{
			outWorldId = binding.worldId;
			outEntity = binding.entity;
		}
	}

	return outWorldId.IsValid() && !outEntity.IsNull();
}

bool ServerSessionSystem::QueuePlayerEntityDespawn(
	FrameworkRuntime& framework,
	const SessionFlow& flow,
	WorldId executionWorldId) noexcept
{
	WorldId worldId = WorldId::Invalid();
	Entity entity = Entity::Null();
	if (!TryResolvePlayerEntityLocation(framework, flow, worldId, entity))
	{
		return false;
	}

	if (executionWorldId.IsValid() && worldId != executionWorldId)
	{
		FWLOG_ERROR(
			kLogCategory,
			"Disconnect despawn scope mismatch (sid=%u, targetWorldId=%u, executionWorldId=%u, entity=%u.%u)",
			flow.sessionId,
			worldId.GetRaw(),
			executionWorldId.GetRaw(),
			entity.id,
			entity.generation);
		return false;
	}

	WorldInstance* const world = framework.FindWorld(worldId);
	if (world == nullptr)
	{
		return false;
	}

	world->GetRuntime().DeferredDestroyEntityIfAlive(entity);
	return true;
}

void ServerSessionSystem::ReleaseUnboundPlayerNetId(
	FrameworkRuntime& framework,
	NetId netId) noexcept
{
	if (!netId.IsValid() || !framework.IsNetIdAlive(netId))
	{
		return;
	}

	(void)framework.UnbindNetEntity(netId);
	framework.FreeNetId(netId);
}

bool ServerSessionSystem::Initialize()
{
	_packetHandlerCtx = PacketHandlerContext
	{
		.network             = &_network,
		.framework           = &_framework,
		.sessionFlow         = &_sessionFlowController,
		.characterData       = &_characterDataService,
		.characterSpawn      = &_characterSpawnService,
		.worldTransitionSink = &_worldTransitionSink,
		.sessionSystem       = this,
		.database            = _database,
		.partyCommandQueue   = _partyCommandQueue,
		.networkTiming       = &_networkTiming
	};
	PacketHandlerContext::Initialize(_packetHandlerCtx);

	_framework.SetDynamicTaskScopeResolver(this);
	_network.SetIOSink(_framework.GetIOSink());

	if (!_network.Initialize())
	{
		_framework.SetDynamicTaskScopeResolver(nullptr);
		FWLOG_FATAL(kLogCategory, "Network runtime initialize failed");
		return false;
	}

	DynamicTaskTypeId disconnectedTypeId{ InvalidDynamicTaskTypeId };
	RegisterServerPacketHandlers(
		_framework.GetDynamicTaskTypeRegistry(),
		_framework.GetExecutionSourceRegistry(),
		_network,
		disconnectedTypeId);

	_framework.SetNetworkBackend(&_network.GetNetworkBackend());
	_network.GetIocpBackend().SetDisconnectedTaskTypeId(disconnectedTypeId);

	_network.Start();
	return true;
}

void ServerSessionSystem::Shutdown() noexcept
{
	_framework.SetDynamicTaskScopeResolver(nullptr);
	_network.Shutdown();
	ClearSessionState();
}

void ServerSessionSystem::SetDatabaseBackend(ODBCDatabaseBackend* database) noexcept
{
	_database = database;
	_packetHandlerCtx.database = database;
}

void ServerSessionSystem::SetPartyCommandQueue(
	PartyCommandQueue* partyCommandQueue) noexcept
{
	_partyCommandQueue = partyCommandQueue;
	_packetHandlerCtx.partyCommandQueue = partyCommandQueue;
}

void ServerSessionSystem::ClearSessionState() noexcept
{
	_characterSpawnService.Clear();
	_sessionFlowController.Clear();
	_networkTiming.Clear();
	_pendingInitialEntries.clear();
	_nextInitialEntryTransferId = 1;
}

void ServerSessionSystem::HandleSessionDisconnected(
	SessionId sessionId,
	SessionCloseReason reason,
	WorldId executionWorldId) noexcept
{
	FWLOG_WARN(kLogCategory,
		"HandleSessionDisconnected entry (sid=%u, reason=%d, executionWorldId=%u)",
		sessionId,
		static_cast<int>(reason),
		executionWorldId.GetRaw());

	const SessionFlow* const existingFlow =
		_sessionFlowController.FindFlow(sessionId);
	const SessionFlow flowSnapshot =
		existingFlow != nullptr ? *existingFlow : SessionFlow{};
	const NetId controlledNetId = flowSnapshot.controlledNetId;

	(void)_characterSpawnService.CancelPendingSpawn(sessionId);
	const bool despawnQueued =
		existingFlow != nullptr &&
		QueuePlayerEntityDespawn(_framework, flowSnapshot, executionWorldId);
	if (!despawnQueued)
	{
		ReleaseUnboundPlayerNetId(_framework, controlledNetId);
	}

	(void)_framework.RemovePresence(sessionId, 0.0);
	_pendingInitialEntries.erase(sessionId);
	_networkTiming.RemoveSession(sessionId);
	_worldTransitionSink.OnSessionDisconnected(sessionId);

	if (_sessionFlowController.FindFlow(sessionId) != nullptr)
	{
		DisconnectRequested command{};
		command.reason = reason;
		(void)_sessionFlowController.Dispatch(sessionId, command);
		_sessionFlowController.OnSessionDisconnected(sessionId);
	}

	(void)_network.GetSessionManager().MarkClosed(sessionId, reason);
	(void)_network.GetSessionManager().RemoveSession(sessionId);
}

void ServerSessionSystem::BeginSendStage() noexcept
{
	_network.BeginSendStage();
}

void ServerSessionSystem::StageTimeSyncPackets(uint64_t serverFrame)
{
	std::vector<SessionId> sessionIds;
	_network.GetSessionManager().FillSessionIds(sessionIds);

	uint32_t gatedOutCount = 0;
	uint32_t probeSkippedCount = 0;
	uint32_t stagedCount = 0;
	uint32_t stageFailedCount = 0;

	for (SessionId sessionId : sessionIds)
	{
		// 핸드셰이크/로그인 단계의 세션, 월드 미바인딩 세션, 종료 중인 세션은
		// time sync 대상에서 제외한다.
		// 1) 클라가 SC_TIME_SYNC 패킷 ID를 아직 디스패치할 수 없는 시점에
		//    프레임마다 발사되어 ProtocolError로 끊기는 사고를 막는다.
		// 2) NetworkTimingService 내부 _sessions 맵에 미준비 세션의 엔트리가
		//    누적되는 것을 방지한다(TryBuildProbe는 게이트 통과 세션만 emplace).
		if (!_sessionFlowController.CanAcceptGameplay(sessionId))
		{
			++gatedOutCount;
			FWLOG_INFO(kLogCategory,
				"StageTimeSyncPackets gated (sid=%u, stateId=%d, serverFrame=%llu)",
				sessionId,
				static_cast<int>(_sessionFlowController.GetState(sessionId)),
				static_cast<unsigned long long>(serverFrame));
			continue;
		}

		NetworkTimeProbe probe{};
		if (!_networkTiming.TryBuildProbe(sessionId, serverFrame, probe))
		{
			++probeSkippedCount;
			continue;
		}

		const bool staged = ServerPacketStager::StageTimeSyncPacketToSession(
			_network,
			sessionId,
			probe.probeSeq,
			probe.serverSendTimeMs,
			probe.serverFrame);
		if (staged)
		{
			++stagedCount;
			FWLOG_INFO(kLogCategory,
				"StageTimeSyncPackets staged (sid=%u, probeSeq=%u, serverSendTimeMs=%u, serverFrame=%llu)",
				sessionId,
				probe.probeSeq,
				probe.serverSendTimeMs,
				static_cast<unsigned long long>(probe.serverFrame));
		}
		else
		{
			++stageFailedCount;
			FWLOG_WARN(kLogCategory,
				"StageTimeSyncPackets stage failed (sid=%u, probeSeq=%u, serverSendTimeMs=%u, serverFrame=%llu)",
				sessionId,
				probe.probeSeq,
				probe.serverSendTimeMs,
				static_cast<unsigned long long>(probe.serverFrame));
		}
	}

	if (!sessionIds.empty())
	{
		FWLOG_INFO(kLogCategory,
			"StageTimeSyncPackets summary (serverFrame=%llu, totalSessions=%zu, gatedOut=%u, probeSkipped=%u, staged=%u, stageFailed=%u)",
			static_cast<unsigned long long>(serverFrame),
			sessionIds.size(),
			gatedOutCount,
			probeSkippedCount,
			stagedCount,
			stageFailedCount);
	}
}

void ServerSessionSystem::FlushOutbound()
{
	_network.FlushSendStage();
}

bool ServerSessionSystem::BeginInitialWorldEntry(
	SessionId sessionId,
	WorldId worldId,
	Entity entity,
	NetId playerNetId,
	CharacterId characterId)
{
	if (sessionId == 0 ||
		!worldId.IsValid() ||
		entity.IsNull() ||
		!playerNetId.IsValid())
	{
		return false;
	}

	const WorldInstance* const world = _framework.FindWorld(worldId);
	if (world == nullptr || world->GetDef() == nullptr)
	{
		FWLOG_ERROR(kLogCategory, "Initial entry begin failed: missing world def (sid=%u, worldId=%u)",
			sessionId, worldId.GetRaw());
		return false;
	}

	if (_pendingInitialEntries.contains(sessionId))
	{
		FWLOG_WARN(kLogCategory, "Initial entry begin rejected: duplicate pending entry (sid=%u)",
			sessionId);
		return false;
	}

	const WorldDef* const worldDef = world->GetDef();
	const TransferId transferId = _nextInitialEntryTransferId++;

	ServerWorldTransitionBeginPacket transition{};
	transition.transferId            = transferId;
	transition.requestId             = 0;
	transition.sourceWorldDefId      = 0;
	transition.sourceWorldId         = 0;
	transition.targetWorldDefId      = static_cast<uint32_t>(worldDef->id);
	transition.targetWorldId         = worldId.GetRaw();
	transition.mapResourceId         = worldDef->map.resourceId;
	transition.playerNetId           = playerNetId.GetRaw();
	transition.clearExistingObjects  = true;
	transition.waitClientReady       = true;
	transition.usedFallback          = false;
	transition.reason                = 0;

	if (!ServerPacketStager::StageWorldTransitionBeginPacket(
		_network,
		sessionId,
		transition))
	{
		FWLOG_ERROR(kLogCategory, "Initial entry begin packet stage failed (sid=%u, transferId=%u)",
			sessionId, transferId);
		return false;
	}

	if (SessionFlow* const flow = _sessionFlowController.FindFlow(sessionId))
	{
		flow->pendingTransferId    = transferId;
		flow->pendingTargetWorldId = worldId;
	}

	_pendingInitialEntries[sessionId] =
		PendingInitialEntry
		{
			.transferId  = transferId,
			.sessionId   = sessionId,
			.worldId     = worldId,
			.entity      = entity,
			.playerNetId = playerNetId,
			.characterId = characterId
		};

	return true;
}

InitialWorldReadyResult ServerSessionSystem::MarkInitialWorldReady(
	SessionId sessionId,
	TransferId transferId,
	double nowSec,
	std::span<const SessionId> additionalExcludedSessions)
{
	const auto it = _pendingInitialEntries.find(sessionId);
	if (it == _pendingInitialEntries.end())
	{
		return InitialWorldReadyResult::NotInitialEntry;
	}

	const PendingInitialEntry pending = it->second;
	if (pending.transferId != transferId)
	{
		FWLOG_WARN(kLogCategory, "Initial world ready rejected: transfer mismatch (sid=%u, expected=%u, actual=%u)",
			sessionId, pending.transferId, transferId);
		return InitialWorldReadyResult::Rejected;
	}

	CharacterId characterId = CharacterId::None;
	const WorldTransformComp* transform = nullptr;
	if (!ServerReplicationSnapshot::TryGetReplicatedSpawnState(
		_framework,
		pending.worldId,
		pending.entity,
		characterId,
		transform))
	{
		FWLOG_ERROR(kLogCategory, "Initial world ready failed: spawn state unavailable (sid=%u, worldId=%u, netId=%u)",
			sessionId, pending.worldId.GetRaw(), pending.playerNetId.GetRaw());
		return InitialWorldReadyResult::Rejected;
	}

	if (!_sessionFlowController.BindPlayer(sessionId, pending.worldId))
	{
		FWLOG_ERROR(kLogCategory, "Initial world ready failed: session bind (sid=%u, worldId=%u, netId=%u)",
			sessionId, pending.worldId.GetRaw(), pending.playerNetId.GetRaw());
		return InitialWorldReadyResult::Rejected;
	}

	if (!_framework.AttachPresenceToWorld(sessionId, pending.worldId, nowSec))
	{
		FWLOG_ERROR(kLogCategory, "Initial world ready failed: presence attach (sid=%u, worldId=%u, netId=%u)",
			sessionId, pending.worldId.GetRaw(), pending.playerNetId.GetRaw());
		return InitialWorldReadyResult::Rejected;
	}

	ClientWorldReady readyCommand{};
	readyCommand.transferId = transferId;
	const SessionFlowResult flowResult =
		_sessionFlowController.Dispatch(sessionId, readyCommand);
	if (!flowResult.Succeeded())
	{
		FWLOG_ERROR(kLogCategory, "Initial world ready rejected by flow (sid=%u, transferId=%u)",
			sessionId, transferId);
		return InitialWorldReadyResult::Rejected;
	}

	(void)ServerPacketStager::StageSpawnAddPacketToSession(
		_network,
		sessionId,
		pending.playerNetId,
		characterId,
		transform);

	TitleId equippedTitleId{ InvalidTitleId };
	const bool hasPlazaTitle =
		ServerReplicationSnapshot::TryGetPlazaPlayerTitleState(
			_framework,
			pending.worldId,
			pending.entity,
			equippedTitleId);
	if (hasPlazaTitle)
	{
		(void)ServerPacketStager::StageTitleReplicationPacketToSession(
			_network,
			sessionId,
			pending.playerNetId,
			equippedTitleId);
	}

	if (WorldInstance* const world = _framework.FindWorld(pending.worldId))
	{
		ECSView view = world->GetRuntime().MakeView();
		if (const CombatStatStateComp* const stats =
			view.GetComponent<CombatStatStateComp>(pending.entity))
		{
			(void)ServerPacketStager::StageStatPacketToSession(
				_network,
				sessionId,
				pending.playerNetId,
				*stats);
		}

		const GameplayEffectStateComp* const effects =
			view.GetComponent<GameplayEffectStateComp>(pending.entity);
		const GameplayEffectReplicationComp* const effectReplication =
			view.GetComponent<GameplayEffectReplicationComp>(pending.entity);
		if (effects != nullptr && effectReplication != nullptr)
		{
			(void)ServerPacketStager::StageGameplayEffectPacketToSession(
				_network,
				sessionId,
				pending.playerNetId,
				*effects,
				*effectReplication,
				false);
		}
	}

	ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
		_framework,
		_network,
		pending.worldId,
		sessionId,
		pending.playerNetId);

	std::vector<SessionId> worldSessionIds;
	std::vector<SessionId> otherReadySessionIds;
	_sessionFlowController.CollectSessionsInWorld(pending.worldId, worldSessionIds);
	otherReadySessionIds.reserve(worldSessionIds.size());
	for (SessionId worldSessionId : worldSessionIds)
	{
		if (worldSessionId == sessionId ||
			_pendingInitialEntries.contains(worldSessionId) ||
			std::find(
				additionalExcludedSessions.begin(),
				additionalExcludedSessions.end(),
				worldSessionId) != additionalExcludedSessions.end())
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
	if (hasPlazaTitle)
	{
		(void)ServerPacketStager::StageTitleReplicationPacketToSessions(
			_network,
			std::span<const SessionId>(
				otherReadySessionIds.data(),
				otherReadySessionIds.size()),
			pending.playerNetId,
			equippedTitleId);
	}

	_pendingInitialEntries.erase(it);
	return InitialWorldReadyResult::Accepted;
}

void ServerSessionSystem::AppendPendingInitialEntrySessions(
	std::vector<SessionId>& outSessionIds) const
{
	outSessionIds.reserve(outSessionIds.size() + _pendingInitialEntries.size());
	for (const auto& [sessionId, pending] : _pendingInitialEntries)
	{
		(void)pending;
		outSessionIds.push_back(sessionId);
	}
}

bool ServerSessionSystem::TryResolveScope(
	const DynamicTaskRequest& request,
	std::span<const WorldId> worldIdByScope,
	ExecScopeId& outScopeId) const noexcept
{
	outScopeId = InvalidExecScopeId;

	if (request.targetKind != DynamicTaskTargetKind::SessionCurrentWorld &&
		request.targetKind != DynamicTaskTargetKind::SessionCurrentWorldOrExplicitScope &&
		request.targetKind != DynamicTaskTargetKind::TypeDefault)
	{
		return false;
	}

	const SessionId sessionId = static_cast<SessionId>(request.sessionId);
	WorldId targetWorldId =
		_sessionFlowController.FindCurrentWorldId(sessionId);

	if (!targetWorldId.IsValid())
	{
		if (const SessionFlow* const flow = _sessionFlowController.FindFlow(sessionId))
		{
			Entity entity = Entity::Null();
			(void)TryResolvePlayerEntityLocation(
				_framework,
				*flow,
				targetWorldId,
				entity);
		}
	}

	if (!targetWorldId.IsValid())
	{
		if (const PendingSessionCharacterSpawn* const pending =
			_characterSpawnService.FindPendingSpawn(sessionId))
		{
			targetWorldId = pending->worldId;
		}
	}

	if (!targetWorldId.IsValid())
	{
		return false;
	}

	for (ExecScopeId scopeId = 0;
		scopeId < static_cast<ExecScopeId>(worldIdByScope.size());
		++scopeId)
	{
		if (worldIdByScope[scopeId] == targetWorldId)
		{
			outScopeId = scopeId;
			return true;
		}
	}

	return false;
}
