#include "pch.h"
#include "SessionFlowController.h"

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
}

void SessionFlowController::OnSessionConnected(SessionId sessionId)
{
	(void)EnsureEntry(sessionId);
}

void SessionFlowController::OnSessionDisconnected(SessionId sessionId)
{
	_sessions.erase(sessionId);
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
		it->second.flow.stateId = SessionStateId::Connected;
	}
	return &it->second;
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
			flow.selectedCharacterId = command.characterId;
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
			flow.playerEntity = command.entity;
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
	result.code = transition.code;
	result.currentState = entry.flow.stateId;
	result.closeReason = transition.closeReason;

	if (!transition.accepted)
	{
		return result;
	}

	entry.flow.stateId = transition.nextState;
	result.currentState = entry.flow.stateId;

	if (transition.closeRequested)
	{
		result.closeReason = transition.closeReason;
	}

	return result;
}
