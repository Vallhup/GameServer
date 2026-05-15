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

namespace
{
	constexpr const char* kLogCategory = "SessionSystem";
}

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
		.framework = &_framework
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
		.sessionSystem       = this
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

void ServerSessionSystem::ClearSessionState() noexcept
{
	_characterSpawnService.Clear();
	_sessionFlowController.Clear();
	_pendingInitialEntries.clear();
	_nextInitialEntryTransferId = 1;
}

void ServerSessionSystem::HandleSessionDisconnected(
	SessionId sessionId,
	SessionCloseReason reason) noexcept
{
	(void)_characterSpawnService.CancelPendingSpawn(sessionId);
	(void)_framework.RemovePresence(sessionId, 0.0);
	_pendingInitialEntries.erase(sessionId);
	_worldTransitionSink.OnSessionDisconnected(sessionId);

	if (_sessionFlowController.FindFlow(sessionId) != nullptr)
	{
		DisconnectRequested command{};
		command.reason = reason;
		(void)_sessionFlowController.Dispatch(sessionId, command);
		_sessionFlowController.OnSessionDisconnected(sessionId);
	}
}

void ServerSessionSystem::BeginSendStage() noexcept
{
	_network.BeginSendStage();
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
		request.targetKind != DynamicTaskTargetKind::TypeDefault)
	{
		return false;
	}

	const WorldId currentWorldId =
		_sessionFlowController.FindCurrentWorldId(
			static_cast<SessionId>(request.sessionId));
	if (!currentWorldId.IsValid())
	{
		return false;
	}

	for (ExecScopeId scopeId = 0;
		scopeId < static_cast<ExecScopeId>(worldIdByScope.size());
		++scopeId)
	{
		if (worldIdByScope[scopeId] == currentWorldId)
		{
			outScopeId = scopeId;
			return true;
		}
	}

	return false;
}
