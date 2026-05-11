#include "pch.h"
#include "SessionFlowController.h"

#include <algorithm>

SessionFlowController::SessionFlowController(SessionFlowDependencies dependencies)
	: _dependencies(dependencies)
{
	RegisterDefaultTransitions();
}

void SessionFlowController::SetDependencies(SessionFlowDependencies dependencies) noexcept
{
	_dependencies = dependencies;
}

void SessionFlowController::Clear() noexcept
{
	_sessions.clear();

	std::unique_lock lock{ _indexMutex };
	_sessionByNetId.clear();
	_sessionsByWorld.clear();
}

void SessionFlowController::OnSessionConnected(SessionId sessionId)
{
	(void)EnsureEntry(sessionId);
}

void SessionFlowController::OnSessionDisconnected(SessionId sessionId)
{
	const auto it = _sessions.find(sessionId);
	if (it != _sessions.end())
	{
		const SessionFlow& flow = it->second.flow;
		if (flow.HasBinding())
		{
			std::unique_lock lock{ _indexMutex };
			IndexUnbind_Locked(sessionId, flow.controlledNetId, flow.currentWorldId);
		}
		_sessions.erase(it);
	}
}

SessionFlowResult SessionFlowController::Dispatch(
	SessionId sessionId,
	const ISessionCommand& command)
{
	Entry* const entry = EnsureEntry(sessionId);
	if (entry == nullptr)
	{
		return SessionFlowResult
		{
			.code = SessionFlowResultCode::InvalidSession
		};
	}

	const SessionStateId previous = entry->flow.stateId;
	SessionFlowContext ctx(entry->flow, _dependencies);
	const TransitionResult transition =
		_transitions.Dispatch(ctx, previous, command);

	SessionFlowResult result = ApplyTransition(*entry, transition);
	result.previousState = previous;
	return result;
}

bool SessionFlowController::CanAcceptGameplay(SessionId sessionId) const
{
	return GetState(sessionId) == SessionStateId::InGame;
}

bool SessionFlowController::CanAcceptReplicationAck(SessionId sessionId) const
{
	return GetState(sessionId) == SessionStateId::InGame;
}

SessionStateId SessionFlowController::GetState(SessionId sessionId) const
{
	const SessionFlow* const flow = FindFlow(sessionId);
	return flow != nullptr ? flow->stateId : SessionStateId::Closing;
}

SessionFlow* SessionFlowController::FindFlow(SessionId sessionId) noexcept
{
	const auto it = _sessions.find(sessionId);
	return it != _sessions.end() ? &it->second.flow : nullptr;
}

const SessionFlow* SessionFlowController::FindFlow(SessionId sessionId) const noexcept
{
	const auto it = _sessions.find(sessionId);
	return it != _sessions.end() ? &it->second.flow : nullptr;
}

// ---------------------------------------------------------------
// 바인딩 인덱스 조회 API
// ---------------------------------------------------------------

SessionId SessionFlowController::FindOwnerSession(NetId controlledNetId) const noexcept
{
	std::shared_lock lock{ _indexMutex };
	const auto it = _sessionByNetId.find(controlledNetId);
	return it != _sessionByNetId.end() ? it->second : 0;
}

NetId SessionFlowController::FindControlledNetId(SessionId sessionId) const noexcept
{
	const SessionFlow* const flow = FindFlow(sessionId);
	return flow != nullptr ? flow->controlledNetId : NetId::Invalid();
}

WorldId SessionFlowController::FindCurrentWorldId(SessionId sessionId) const noexcept
{
	const SessionFlow* const flow = FindFlow(sessionId);
	return flow != nullptr ? flow->currentWorldId : WorldId::Invalid();
}

bool SessionFlowController::HasBinding(SessionId sessionId) const noexcept
{
	const SessionFlow* const flow = FindFlow(sessionId);
	return flow != nullptr && flow->HasBinding();
}

void SessionFlowController::CollectSessionsInWorld(
	WorldId worldId,
	std::vector<SessionId>& outSessionIds) const
{
	outSessionIds.clear();
	if (!worldId.IsValid())
	{
		return;
	}

	std::shared_lock lock{ _indexMutex };
	const auto it = _sessionsByWorld.find(worldId);
	if (it != _sessionsByWorld.end())
	{
		outSessionIds = it->second;
		std::sort(outSessionIds.begin(), outSessionIds.end());
	}
}

// ---------------------------------------------------------------
// 바인딩 변경 API
// ---------------------------------------------------------------

bool SessionFlowController::BindPlayer(
	SessionId sessionId,
	NetId controlledNetId,
	WorldId currentWorldId) noexcept
{
	if (sessionId == 0 || !controlledNetId.IsValid() || !currentWorldId.IsValid())
	{
		return false;
	}

	SessionFlow* const flow = FindFlow(sessionId);
	if (flow == nullptr)
	{
		return false;
	}

	// 기존 바인딩이 있다면 역인덱스에서 먼저 제거
	const bool hadBinding = flow->HasBinding();
	const NetId oldNetId = flow->controlledNetId;
	const WorldId oldWorldId = flow->currentWorldId;

	flow->controlledNetId = controlledNetId;
	flow->currentWorldId  = currentWorldId;

	std::unique_lock lock{ _indexMutex };
	if (hadBinding)
	{
		IndexUnbind_Locked(sessionId, oldNetId, oldWorldId);
	}
	IndexBind_Locked(sessionId, controlledNetId, currentWorldId);
	return true;
}

bool SessionFlowController::UnbindPlayer(SessionId sessionId) noexcept
{
	SessionFlow* const flow = FindFlow(sessionId);
	if (flow == nullptr || !flow->HasBinding())
	{
		return false;
	}

	const NetId oldNetId     = flow->controlledNetId;
	const WorldId oldWorldId = flow->currentWorldId;

	flow->controlledNetId = NetId::Invalid();
	flow->currentWorldId  = WorldId::Invalid();

	std::unique_lock lock{ _indexMutex };
	IndexUnbind_Locked(sessionId, oldNetId, oldWorldId);
	return true;
}

bool SessionFlowController::UpdatePlayerWorld(
	SessionId sessionId,
	WorldId newWorldId) noexcept
{
	if (!newWorldId.IsValid())
	{
		return false;
	}

	SessionFlow* const flow = FindFlow(sessionId);
	if (flow == nullptr || !flow->HasBinding())
	{
		return false;
	}

	const WorldId oldWorldId = flow->currentWorldId;
	flow->currentWorldId = newWorldId;

	std::unique_lock lock{ _indexMutex };
	IndexUpdateWorld_Locked(sessionId, oldWorldId, newWorldId);
	return true;
}

// ---------------------------------------------------------------
// 내부 헬퍼
// ---------------------------------------------------------------

SessionFlowController::Entry* SessionFlowController::EnsureEntry(SessionId sessionId)
{
	if (sessionId == 0)
	{
		return nullptr;
	}

	const auto [it, inserted] = _sessions.try_emplace(sessionId);
	if (inserted)
	{
		it->second.flow.sessionId = sessionId;
		it->second.flow.stateId   = SessionStateId::Connected;
	}
	return &it->second;
}

void SessionFlowController::IndexBind_Locked(
	SessionId sessionId,
	NetId controlledNetId,
	WorldId currentWorldId)
{
	_sessionByNetId[controlledNetId] = sessionId;
	_sessionsByWorld[currentWorldId].push_back(sessionId);
}

void SessionFlowController::IndexUnbind_Locked(
	SessionId sessionId,
	NetId controlledNetId,
	WorldId currentWorldId) noexcept
{
	_sessionByNetId.erase(controlledNetId);

	const auto worldIt = _sessionsByWorld.find(currentWorldId);
	if (worldIt != _sessionsByWorld.end())
	{
		auto& vec = worldIt->second;
		vec.erase(std::remove(vec.begin(), vec.end(), sessionId), vec.end());
		if (vec.empty())
		{
			_sessionsByWorld.erase(worldIt);
		}
	}
}

void SessionFlowController::IndexUpdateWorld_Locked(
	SessionId sessionId,
	WorldId oldWorldId,
	WorldId newWorldId) noexcept
{
	// 구 world 버킷에서 제거
	const auto oldIt = _sessionsByWorld.find(oldWorldId);
	if (oldIt != _sessionsByWorld.end())
	{
		auto& vec = oldIt->second;
		vec.erase(std::remove(vec.begin(), vec.end(), sessionId), vec.end());
		if (vec.empty())
		{
			_sessionsByWorld.erase(oldIt);
		}
	}

	// 신 world 버킷에 추가
	_sessionsByWorld[newWorldId].push_back(sessionId);
}

void SessionFlowController::RegisterDefaultTransitions()
{
	_transitions.Register<LoginRequested>(
		SessionStateId::Connected,
		SessionCommandId::LoginRequested,
		[](SessionFlowContext& ctx, const LoginRequested& command)
		{
			(void)command;
			return TransitionResult::To(SessionStateId::Authenticating);
		});

	_transitions.Register<LoginSucceeded>(
		SessionStateId::Authenticating,
		SessionCommandId::LoginSucceeded,
		[](SessionFlowContext& ctx, const LoginSucceeded& command)
		{
			ctx.Flow().playerNetId = command.playerNetId;
			return TransitionResult::To(SessionStateId::AwaitingCharacterSelect);
		});

	_transitions.Register<LoginFailed>(
		SessionStateId::Authenticating,
		SessionCommandId::LoginFailed,
		[](SessionFlowContext& ctx, const LoginFailed& command)
		{
			(void)ctx;
			(void)command;
			return TransitionResult::Close(SessionCloseReason::ProtocolError);
		});

	_transitions.Register<CharacterSelectRequested>(
		SessionStateId::AwaitingCharacterSelect,
		SessionCommandId::CharacterSelectRequested,
		[](SessionFlowContext& ctx, const CharacterSelectRequested& command)
		{
			ctx.Flow().selectedCharacterId = command.characterId;
			return TransitionResult::To(SessionStateId::CharacterDataLoading);
		});

	_transitions.Register<CharacterDataLoaded>(
		SessionStateId::CharacterDataLoading,
		SessionCommandId::CharacterDataLoaded,
		[](SessionFlowContext& ctx, const CharacterDataLoaded& command)
		{
			SessionFlow& flow = ctx.Flow();
			flow.selectedCharacterId  = command.characterId;
			flow.pendingTargetWorldId = command.worldId;
			return TransitionResult::To(SessionStateId::SpawningCharacter);
		});

	_transitions.Register<CharacterDataLoadFailed>(
		SessionStateId::CharacterDataLoading,
		SessionCommandId::CharacterDataLoadFailed,
		[](SessionFlowContext& ctx, const CharacterDataLoadFailed& command)
		{
			(void)ctx;
			(void)command;
			return TransitionResult::Close(SessionCloseReason::ProtocolError);
		});

	_transitions.Register<CharacterSpawnConfirmed>(
		SessionStateId::SpawningCharacter,
		SessionCommandId::CharacterSpawnConfirmed,
		[](SessionFlowContext& ctx, const CharacterSpawnConfirmed& command)
		{
			SessionFlow& flow = ctx.Flow();
			flow.playerWorldId = command.worldId;
			flow.playerEntity  = command.entity;
			if (!flow.playerNetId.IsValid())
			{
				flow.playerNetId = command.netId;
			}
			return TransitionResult::To(SessionStateId::AwaitingClientWorldReady);
		});

	_transitions.Register<CharacterSpawnFailed>(
		SessionStateId::SpawningCharacter,
		SessionCommandId::CharacterSpawnFailed,
		[](SessionFlowContext& ctx, const CharacterSpawnFailed& command)
		{
			(void)ctx;
			(void)command;
			return TransitionResult::Close(SessionCloseReason::ProtocolError);
		});

	_transitions.Register<ClientWorldReady>(
		SessionStateId::AwaitingClientWorldReady,
		SessionCommandId::ClientWorldReady,
		[](SessionFlowContext& ctx, const ClientWorldReady& command)
		{
			(void)command;
			ctx.Flow().pendingTransferId = 0;
			return TransitionResult::To(SessionStateId::InGame);
		});

	// DisconnectRequested: 현재는 Connected 상태에만 등록
	// (Step 4 Hierarchical State Machine 도입 시 Live 슈퍼상태로 격상)
	_transitions.Register<DisconnectRequested>(
		SessionStateId::Connected,
		SessionCommandId::DisconnectRequested,
		[](SessionFlowContext& ctx, const DisconnectRequested& command)
		{
			(void)ctx;
			return TransitionResult::Close(command.reason);
		});
}

SessionFlowResult SessionFlowController::ApplyTransition(
	Entry& entry,
	const TransitionResult& transition)
{
	SessionFlowResult result{};
	result.code         = transition.code;
	result.currentState = entry.flow.stateId;
	result.closeReason  = transition.closeReason;

	if (!transition.accepted)
	{
		return result;
	}

	entry.flow.stateId  = transition.nextState;
	result.currentState = entry.flow.stateId;

	if (transition.closeRequested)
	{
		result.closeReason = transition.closeReason;
	}

	return result;
}
