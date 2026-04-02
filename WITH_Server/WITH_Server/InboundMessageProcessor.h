#pragma once

#include <vector>

#include "FrameworkRuntime.h"
#include "InboundMessage.h"
#include "NetworkRuntime.h"
#include "SessionBindingRegistry.h"

class PlayerEntryService;

class InboundMessageProcessor final {
public:
	struct Dependencies
	{
		NetworkRuntime* network{ nullptr };
		PlayerEntryService* playerEntryService{ nullptr };
		SessionBindingRegistry* sessionBindings{ nullptr };
	};

public:
	explicit InboundMessageProcessor(Dependencies deps = {});

	void SetDependencies(Dependencies deps) noexcept;

	void Process(
		const std::vector<InboundMessage>& inputMessages,
		std::vector<InboundMessage>& outRemainingMessages);

private:
	bool TryHandleMessage(const InboundMessage& message);

	bool HandleConnected(const InboundMessage& message);
	bool HandleDisconnected(const InboundMessage& message);
	bool HandleLoginPacket(const InboundMessage& message);
	bool HandleProtocolError(const InboundMessage& message);
	bool HandleMovePacket(const InboundMessage& message);
	bool HandleAttackPacket(const InboundMessage& message);
	bool HandleDodgePacket(const InboundMessage& message);
	bool HandleGuardPacket(const InboundMessage& message);
	bool HandleParryPacket(const InboundMessage& message);

	bool ValidateLoginRequest(const InboundMessage& message) const noexcept;
	bool HandleGameplayCommandMock(const InboundMessage& message) noexcept;

	void CompleteLogin(SessionId sessionId);
	void RejectLogin(SessionId sessionId);

private:
	Dependencies _deps;
};
