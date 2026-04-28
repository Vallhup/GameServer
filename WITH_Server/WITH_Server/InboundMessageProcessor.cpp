#include "pch.h"
#include "InboundMessageProcessor.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>

#include "FrameworkLog.h"
#include "PlayerEntryService.h"
#include "ServerPacketStager.h"
#include "WorldInstance.h"

namespace
{
	constexpr const char* kLogCategory = "InboundProcessor";
	constexpr uint32_t kDefaultCommandSequence = 0;
	constexpr uint32_t kWorldTransitionRejectReasonServerUnavailable = 1;
	constexpr uint32_t kWorldTransitionRejectReasonRequestRejected = 2;

	PlayerMoveCommandPayload MakeMovePayload(const InboundMoveData& moveData) noexcept
	{
		return PlayerMoveCommandPayload{
			.inputX = static_cast<float>(moveData.inputX),
			.inputZ = static_cast<float>(moveData.inputZ),
			.yaw = moveData.yaw,
			.isRun = moveData.isRun ? uint8_t{ 1 } : uint8_t{ 0 }
		};
	}

	PlayerDirectionCommandPayload MakeDirectionPayload(
		const InboundDirectionData& directionData) noexcept
	{
		return PlayerDirectionCommandPayload{
			.dirX = directionData.dirX,
			.dirZ = directionData.dirZ
		};
	}

	PlayerGuardCommandPayload MakeGuardPayload(
		const InboundGuardData& guardData) noexcept
	{
		return PlayerGuardCommandPayload{
			.pressed = guardData.input ? uint8_t{ 1 } : uint8_t{ 0 }
		};
	}
}

size_t InboundMessageProcessor::CommandRouteKeyHash::operator()(
	const CommandRouteKey& key) const noexcept
{
	size_t seed = std::hash<WorldId>()(key.worldId);
	seed ^= std::hash<SessionId>()(key.sourceSessionId) +
		0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
	seed ^= std::hash<NetId>()(key.targetNetId) +
		0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
	return seed;
}

InboundMessageProcessor::InboundMessageProcessor(Dependencies deps)
	: _deps(deps)
{
}

void InboundMessageProcessor::SetDependencies(Dependencies deps) noexcept
{
	_deps = deps;
}

void InboundMessageProcessor::ClearCommandStats() noexcept
{
	_commandStats.Clear();
}

void InboundMessageProcessor::Process(
	const std::vector<InboundMessage>& inputMessages,
	std::vector<InboundMessage>& outRemainingMessages)
{
	assert(&inputMessages != &outRemainingMessages);

	outRemainingMessages.clear();
	outRemainingMessages.reserve(inputMessages.size());

	StagedCommandMap stagedCommands;

	for (size_t index = 0; index < inputMessages.size(); ++index)
	{
		const InboundMessage& message = inputMessages[index];
		const uint64_t messageOrder = static_cast<uint64_t>(index) + 1;

		if (IsGameplayPacket(message))
		{
			if (!TryStageGameplayCommand(message, messageOrder, stagedCommands))
			{
				outRemainingMessages.push_back(message);
			}
			continue;
		}

		FlushStagedCommands(stagedCommands);

		if (!TryHandleMessage(message))
		{
			outRemainingMessages.push_back(message);
		}
	}

	FlushStagedCommands(stagedCommands);
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

	case InboundMessageKind::WorldTransitionRequestPacket:
		return HandleWorldTransitionRequestPacket(message);

	case InboundMessageKind::WorldTransitionReadyPacket:
		return HandleWorldTransitionReadyPacket(message);

	case InboundMessageKind::ProtocolError:
		return HandleProtocolError(message);

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

	if (_deps.playerEntryService != nullptr)
	{
		(void)_deps.playerEntryService->CancelEntry(message.sessionId);
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

bool InboundMessageProcessor::HandleWorldTransitionRequestPacket(
	const InboundMessage& message)
{
	if (_deps.worldTransitionSink == nullptr)
	{
		if (_deps.network != nullptr)
		{
			(void)ServerPacketStager::StageWorldTransitionRejectedPacket(
				*_deps.network,
				message.sessionId,
				message.payload.worldTransitionRequest.requestId,
				kWorldTransitionRejectReasonServerUnavailable);
		}
		return true;
	}

	const TransferId transferId =
		_deps.worldTransitionSink->RequestDemoWorldTransition(
			message.sessionId,
			message.payload.worldTransitionRequest.requestId);
	if (transferId == 0)
	{
		FWLOG_WARN(kLogCategory, "WorldTransition request rejected (sid=%u, requestId=%u)",
			message.sessionId, message.payload.worldTransitionRequest.requestId);
		if (_deps.network != nullptr)
		{
			(void)ServerPacketStager::StageWorldTransitionRejectedPacket(
				*_deps.network,
				message.sessionId,
				message.payload.worldTransitionRequest.requestId,
				kWorldTransitionRejectReasonRequestRejected);
		}
	}
	return true;
}

bool InboundMessageProcessor::HandleWorldTransitionReadyPacket(
	const InboundMessage& message)
{
	if (_deps.worldTransitionSink != nullptr)
	{
		(void)_deps.worldTransitionSink->MarkClientWorldTransitionReady(
			message.sessionId,
			message.payload.worldTransitionReady.transferId);
	}
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

bool InboundMessageProcessor::ValidateLoginRequest(
	const InboundMessage& message) const noexcept
{
	if (_deps.sessionBindings != nullptr &&
		_deps.sessionBindings->HasBinding(message.sessionId))
	{
		return false;
	}

	if (_deps.playerEntryService != nullptr &&
		_deps.playerEntryService->FindContext(message.sessionId) != nullptr)
	{
		return false;
	}

	// Future work:
	// - account/auth token validation
	// - duplicate login policy
	// - DB preload
	return true;
}

bool InboundMessageProcessor::IsGameplayPacket(
	const InboundMessage& message) const noexcept
{
	switch (message.kind) {
	case InboundMessageKind::MovePacket:
	case InboundMessageKind::AttackPacket:
	case InboundMessageKind::DodgePacket:
	case InboundMessageKind::GuardPacket:
	case InboundMessageKind::ParryPacket:
		return true;
	default:
		return false;
	}
}

bool InboundMessageProcessor::TryStageGameplayCommand(
	const InboundMessage& message,
	uint64_t messageOrder,
	StagedCommandMap& stagedCommands)
{
	CommandRouteKey routeKey{};
	if (!TryResolveCommandRoute(message.sessionId, routeKey))
	{
		return true;
	}

	WorldCommand command{};
	if (!TryBuildWorldCommand(message, routeKey, command))
	{
		return false;
	}

	StageWorldCommand(
		messageOrder,
		routeKey,
		std::move(command),
		stagedCommands);
	return true;
}

bool InboundMessageProcessor::TryResolveCommandRoute(
	SessionId sessionId,
	CommandRouteKey& outRouteKey) noexcept
{
	outRouteKey = CommandRouteKey{};

	if (_deps.framework == nullptr ||
		_deps.sessionBindings == nullptr)
	{
		_commandStats.AddMissingSessionBinding();
		return false;
	}

	const NetId netId = _deps.sessionBindings->FindControlledNetId(sessionId);
	if (!netId.IsValid())
	{
		_commandStats.AddMissingSessionBinding();
		return false;
	}

	const NetBindingLocation netBinding =
		_deps.framework->FindNetBinding(netId);
	if (!netBinding.IsValid())
	{
		_commandStats.AddNetBindingNotFound();
		return false;
	}

	outRouteKey.worldId = netBinding.worldId;
	outRouteKey.sourceSessionId = sessionId;
	outRouteKey.targetNetId = netId;
	return true;
}

bool InboundMessageProcessor::TryBuildWorldCommand(
	const InboundMessage& message,
	const CommandRouteKey& routeKey,
	WorldCommand& outCommand) const
{
	switch (message.kind) {
	case InboundMessageKind::MovePacket:
		outCommand = MakePlayerMoveCommand(
			routeKey.sourceSessionId,
			routeKey.targetNetId,
			kDefaultCommandSequence,
			MakeMovePayload(message.payload.move));
		return true;

	case InboundMessageKind::AttackPacket:
		// CS_ATTACK currently has one packet shape, so map it to LightAttack first.
		outCommand = MakePlayerLightAttackCommand(
			routeKey.sourceSessionId,
			routeKey.targetNetId,
			kDefaultCommandSequence,
			MakeDirectionPayload(message.payload.direction));
		return true;

	case InboundMessageKind::DodgePacket:
		outCommand = MakePlayerDodgeCommand(
			routeKey.sourceSessionId,
			routeKey.targetNetId,
			kDefaultCommandSequence,
			MakeDirectionPayload(message.payload.direction));
		return true;

	case InboundMessageKind::GuardPacket:
		outCommand = MakePlayerGuardCommand(
			routeKey.sourceSessionId,
			routeKey.targetNetId,
			kDefaultCommandSequence,
			MakeGuardPayload(message.payload.guard));
		return true;

	case InboundMessageKind::ParryPacket:
		outCommand = MakePlayerParryCommand(
			routeKey.sourceSessionId,
			routeKey.targetNetId,
			kDefaultCommandSequence,
			MakeDirectionPayload(message.payload.direction));
		return true;

	default:
		return false;
	}
}

void InboundMessageProcessor::StageWorldCommand(
	uint64_t messageOrder,
	const CommandRouteKey& routeKey,
	WorldCommand command,
	StagedCommandMap& stagedCommands)
{
	StagedCommandSet& stagedSet = stagedCommands[routeKey];
	OrderedWorldCommand orderedCommand{
		.worldId = routeKey.worldId,
		.messageOrder = messageOrder,
		.command = std::move(command)
	};

	if (IsPlayerMoveCommandType(orderedCommand.command.typeKey))
	{
		if (stagedSet.move.has_value())
		{
			_commandStats.AddMoveCommandOverwritten();
		}
		stagedSet.move = std::move(orderedCommand);
		return;
	}

	if (IsPlayerActionEventCommandType(orderedCommand.command.typeKey))
	{
		if (stagedSet.action.has_value())
		{
			_commandStats.AddActionCommandSuppressed();
			return;
		}
		stagedSet.action = std::move(orderedCommand);
		return;
	}

	if (IsPlayerGuardCommandType(orderedCommand.command.typeKey))
	{
		if (stagedSet.guard.has_value())
		{
			_commandStats.AddGuardCommandOverwritten();
		}
		stagedSet.guard = std::move(orderedCommand);
	}
}

void InboundMessageProcessor::FlushStagedCommands(
	StagedCommandMap& stagedCommands)
{
	if (stagedCommands.empty())
	{
		return;
	}

	std::vector<OrderedWorldCommand> commands;
	commands.reserve(stagedCommands.size() * 3);

	for (auto& [routeKey, stagedSet] : stagedCommands)
	{
		(void)routeKey;

		if (stagedSet.move.has_value())
		{
			commands.push_back(std::move(*stagedSet.move));
		}

		if (stagedSet.action.has_value())
		{
			commands.push_back(std::move(*stagedSet.action));
		}

		if (stagedSet.guard.has_value())
		{
			commands.push_back(std::move(*stagedSet.guard));
		}
	}

	std::sort(
		commands.begin(),
		commands.end(),
		[](const OrderedWorldCommand& lhs, const OrderedWorldCommand& rhs)
		{
			return lhs.messageOrder < rhs.messageOrder;
		});

	for (OrderedWorldCommand& command : commands)
	{
		if (_deps.framework == nullptr)
		{
			_commandStats.AddWorldNotFound();
			continue;
		}

		WorldInstance* const world = _deps.framework->FindWorld(command.worldId);
		if (world == nullptr)
		{
			_commandStats.AddWorldNotFound();
			continue;
		}

		if (!world->GetRuntime().EnqueueWorldCommand(std::move(command.command)))
		{
			_commandStats.AddEnqueueFailed();
		}
	}

	stagedCommands.clear();
}

void InboundMessageProcessor::CompleteLogin(SessionId sessionId)
{
	if (_deps.network == nullptr ||
		_deps.playerEntryService == nullptr)
	{
		return;
	}

	(void)_deps.network->RequestCompleteLogin(sessionId);

	if (!_deps.playerEntryService->BeginAuthenticatedEntry(sessionId))
	{
		RejectLogin(sessionId);
		return;
	}

	// Temporary bootstrap path:
	// until CS_SELECT_CHARACTER is wired, automatically choose Knight so the
	// end-to-end entry flow can run through PlayerEntryService.
	const PlayerEntryResult selectResult =
		_deps.playerEntryService->RequestCharacterSelect(
			sessionId,
			CharacterId::Knight);
	if (!selectResult.Succeeded())
	{
		(void)_deps.playerEntryService->CancelEntry(sessionId);
		RejectLogin(sessionId);
		return;
	}

	// Future work:
	// - replace the temporary Knight auto-select path with CS_SELECT_CHARACTER
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
