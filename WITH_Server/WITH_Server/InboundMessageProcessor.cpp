#include "pch.h"
#include "InboundMessageProcessor.h"

#include <algorithm>

#include "RepComponent.h"
#include "WorldInstance.h"

InboundMessageProcessor::InboundMessageProcessor(Dependencies deps)
	: _deps(deps)
{
}

void InboundMessageProcessor::SetDependencies(Dependencies deps) noexcept
{
	_deps = deps;
}

void InboundMessageProcessor::Process(
	const std::vector<InboundMessage>& inputMessages,
	std::vector<InboundMessage>& outRemainingMessages)
{
	outRemainingMessages.clear();
	outRemainingMessages.reserve(inputMessages.size());

	for (const InboundMessage& message : inputMessages)
	{
		if (!TryHandleMessage(message))
		{
			outRemainingMessages.push_back(message);
		}
	}
}

bool InboundMessageProcessor::TryHandleMessage(const InboundMessage& message)
{
	switch (message.kind) {
	case InboundMessageKind::Connected:
		return HandleConnected(message);

	case InboundMessageKind::Disconnected:
		return HandleDisconnected(message);

	case InboundMessageKind::LoginPacket:
		return HandleLoginPacket(message);

	case InboundMessageKind::ProtocolError:
		return HandleProtocolError(message);

	case InboundMessageKind::MovePacket:
		return HandleMovePacket(message);

	case InboundMessageKind::AttackPacket:
		return HandleAttackPacket(message);

	case InboundMessageKind::DodgePacket:
		return HandleDodgePacket(message);

	case InboundMessageKind::GuardPacket:
		return HandleGuardPacket(message);

	case InboundMessageKind::ParryPacket:
		return HandleParryPacket(message);

	default:
		return false;
	}
}

bool InboundMessageProcessor::HandleConnected(const InboundMessage& message)
{
	(void)message;

	// Future work:
	// - metrics / telemetry
	// - duplicate session policy
	return true;
}

bool InboundMessageProcessor::HandleDisconnected(const InboundMessage& message)
{
	if (_deps.sessionBindings != nullptr)
	{
		(void)_deps.sessionBindings->Unbind(message.sessionId);
	}

	if (_deps.pendingLoginSpawns != nullptr)
	{
		auto& pending = *_deps.pendingLoginSpawns;
		pending.erase(
			std::remove_if(
				pending.begin(),
				pending.end(),
				[&](const PendingLoginSpawn& spawn)
				{
					return spawn.sessionId == message.sessionId;
				}),
			pending.end());
	}

	// Future work:
	// - player/world cleanup orchestration
	// - leave-world replication
	return true;
}

bool InboundMessageProcessor::HandleLoginPacket(const InboundMessage& message)
{
	if (!ValidateLoginRequest(message))
	{
		RejectLogin(message.sessionId);
		return true;
	}

	CompleteLogin(message.sessionId);
	return true;
}

bool InboundMessageProcessor::HandleProtocolError(const InboundMessage& message)
{
	if (_deps.network == nullptr)
	{
		return true;
	}

	(void)_deps.network->RequestClose(
		message.sessionId,
		SessionCloseReason::ProtocolError);
	return true;
}

bool InboundMessageProcessor::HandleMovePacket(const InboundMessage& message)
{
	return HandleGameplayCommandMock(message);
}

bool InboundMessageProcessor::HandleAttackPacket(const InboundMessage& message)
{
	return HandleGameplayCommandMock(message);
}

bool InboundMessageProcessor::HandleDodgePacket(const InboundMessage& message)
{
	return HandleGameplayCommandMock(message);
}

bool InboundMessageProcessor::HandleGuardPacket(const InboundMessage& message)
{
	return HandleGameplayCommandMock(message);
}

bool InboundMessageProcessor::HandleParryPacket(const InboundMessage& message)
{
	return HandleGameplayCommandMock(message);
}

bool InboundMessageProcessor::ValidateLoginRequest(
	const InboundMessage& message) const noexcept
{
	if (_deps.sessionBindings != nullptr &&
		_deps.sessionBindings->HasBinding(message.sessionId))
	{
		return false;
	}

	if (_deps.pendingLoginSpawns != nullptr)
	{
		for (const PendingLoginSpawn& pending : *_deps.pendingLoginSpawns)
		{
			if (pending.sessionId == message.sessionId)
			{
				return false;
			}
		}
	}

	// Future work:
	// - account/auth token validation
	// - duplicate login policy
	// - DB preload
	return true;
}

bool InboundMessageProcessor::HandleGameplayCommandMock(
	const InboundMessage& message) noexcept
{
	if (_deps.sessionBindings == nullptr)
	{
		return true;
	}

	if (!_deps.sessionBindings->HasBinding(message.sessionId))
	{
		return true;
	}

	// Future work:
	// - resolve session -> player/world binding
	// - convert packet-shaped input into world-local gameplay command
	// - enqueue into frame-local command buffer or world inbox
	return true;
}

void InboundMessageProcessor::CompleteLogin(SessionId sessionId)
{
	if (_deps.network == nullptr ||
		_deps.framework == nullptr ||
		_deps.startupWorldId == nullptr ||
		_deps.pendingLoginSpawns == nullptr)
	{
		return;
	}

	const WorldId startupWorldId = *_deps.startupWorldId;
	if (!startupWorldId.IsValid())
	{
		RejectLogin(sessionId);
		return;
	}

	WorldInstance* world = _deps.framework->FindWorld(startupWorldId);
	if (world == nullptr)
	{
		RejectLogin(sessionId);
		return;
	}

	WorldRuntime& runtime = world->GetRuntime();
	const Entity playerEntity = runtime.ReserveEntity();
	if (playerEntity.IsNull())
	{
		RejectLogin(sessionId);
		return;
	}

	runtime.DeferredAddComponent<ReplicatedTag>(playerEntity);

	SpawnTypeComp typeComp =
	{
		.entityType = EntityType::Character,
		.faction = Faction::Player,
		.charType = CharacterType::Knight
	};
	runtime.DeferredUpsertComponent<SpawnTypeComp>(playerEntity, typeComp);

	(void)_deps.network->RequestCompleteLogin(sessionId);
	_deps.pendingLoginSpawns->push_back(
		PendingLoginSpawn{
			sessionId,
			startupWorldId,
			playerEntity
		});

	// Future work:
	// - resolve exact entry point / spawn transform
	// - attach gameplay-specific player components
	// - stage nearby initial replication after spawn confirm
}

void InboundMessageProcessor::RejectLogin(SessionId sessionId)
{
	if (_deps.network == nullptr)
	{
		return;
	}

	// Current protocol cannot represent login failure explicitly.
	// When SC_LOGIN is extended, send failure response first and then close.
	(void)_deps.network->RequestClose(
		sessionId,
		SessionCloseReason::LocalRequested);
}
