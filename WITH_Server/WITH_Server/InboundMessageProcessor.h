#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "FrameworkRuntime.h"
#include "InboundCommandStatsCollector.h"
#include "InboundMessage.h"
#include "NetworkRuntime.h"
#include "PlayerCommand.h"
#include "SessionBindingRegistry.h"
#include "WorldId.h"

class PlayerEntryService;

class InboundMessageProcessor final {
public:
	struct Dependencies
	{
		FrameworkRuntime* framework{ nullptr };
		NetworkRuntime* network{ nullptr };
		PlayerEntryService* playerEntryService{ nullptr };
		SessionBindingRegistry* sessionBindings{ nullptr };
	};

private:
	struct CommandRouteKey
	{
		WorldId worldId{ WorldId::Invalid() };
		SessionId sourceSessionId{ 0 };
		NetId targetNetId{ NetId::Invalid() };

		bool operator==(const CommandRouteKey& rhs) const noexcept
		{
			return
				worldId == rhs.worldId &&
				sourceSessionId == rhs.sourceSessionId &&
				targetNetId == rhs.targetNetId;
		}
	};

	struct CommandRouteKeyHash
	{
		size_t operator()(const CommandRouteKey& key) const noexcept;
	};

	struct OrderedWorldCommand
	{
		WorldId worldId{ WorldId::Invalid() };
		uint64_t messageOrder{ 0 };
		WorldCommand command;
	};

	struct StagedCommandSet
	{
		std::optional<OrderedWorldCommand> move;
		std::optional<OrderedWorldCommand> action;
		std::optional<OrderedWorldCommand> guard;
	};

	using StagedCommandMap =
		std::unordered_map<CommandRouteKey, StagedCommandSet, CommandRouteKeyHash>;

public:
	explicit InboundMessageProcessor(Dependencies deps = {});

	void SetDependencies(Dependencies deps) noexcept;

	void Process(
		const std::vector<InboundMessage>& inputMessages,
		std::vector<InboundMessage>& outRemainingMessages);

	const InboundCommandStatsCollector& GetCommandStats() const noexcept
	{
		return _commandStats;
	}

	void ClearCommandStats() noexcept;

private:
	bool TryHandleMessage(const InboundMessage& message);

	bool HandleConnected(const InboundMessage& message);
	bool HandleDisconnected(const InboundMessage& message);
	bool HandleLoginPacket(const InboundMessage& message);
	bool HandleProtocolError(const InboundMessage& message);

	bool ValidateLoginRequest(const InboundMessage& message) const noexcept;
	bool IsGameplayPacket(const InboundMessage& message) const noexcept;

	bool TryStageGameplayCommand(
		const InboundMessage& message,
		uint64_t messageOrder,
		StagedCommandMap& stagedCommands);

	bool TryResolveCommandRoute(
		SessionId sessionId,
		CommandRouteKey& outRouteKey) noexcept;

	bool TryBuildWorldCommand(
		const InboundMessage& message,
		const CommandRouteKey& routeKey,
		WorldCommand& outCommand) const;

	void StageWorldCommand(
		uint64_t messageOrder,
		const CommandRouteKey& routeKey,
		WorldCommand command,
		StagedCommandMap& stagedCommands);

	void FlushStagedCommands(StagedCommandMap& stagedCommands);

	void CompleteLogin(SessionId sessionId);
	void RejectLogin(SessionId sessionId);

private:
	Dependencies _deps;
	InboundCommandStatsCollector _commandStats;
};
