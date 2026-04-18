#pragma once

#include <cstdint>
#include <type_traits>

#include "Session.h"

enum class InboundMessageKind : uint8_t
{
	Connected,
	Disconnected,

	LoginPacket,
	MovePacket,
	AttackPacket,
	DodgePacket,
	GuardPacket,
	ParryPacket,
	WorldTransitionRequestPacket,
	WorldTransitionReadyPacket,

	ProtocolError,
};

struct InboundEmptyData
{
};

struct InboundMoveData
{
	int32_t inputX{ 0 };
	int32_t inputZ{ 0 };
	float yaw{ 0.0f };
	bool isRun{ false };
	uint8_t reserved[3]{};
};

struct InboundDirectionData
{
	float dirX{ 0.0f };
	float dirZ{ 0.0f };
};

struct InboundGuardData
{
	bool input{ false };
	uint8_t reserved[3]{};
};

struct InboundWorldTransitionRequestData
{
	uint32_t requestId{ 0 };
};

struct InboundWorldTransitionReadyData
{
	uint64_t transferId{ 0 };
};

struct InboundProtocolErrorData
{
	uint16_t packetId{ 0 };
	uint16_t reason{ 0 };
};

union InboundMessagePayload
{
	InboundEmptyData empty;
	InboundMoveData move;
	InboundDirectionData direction;
	InboundGuardData guard;
	InboundWorldTransitionRequestData worldTransitionRequest;
	InboundWorldTransitionReadyData worldTransitionReady;
	InboundProtocolErrorData error;

	constexpr InboundMessagePayload()
		: empty{}
	{
	}
};

struct InboundMessage
{
	SessionId sessionId{ 0 };
	InboundMessageKind kind{ InboundMessageKind::Connected };

	uint16_t packetId{ 0 };
	uint16_t reserved{ 0 };

	InboundMessagePayload payload{};
};

static_assert(std::is_trivially_copyable_v<InboundMessage>);
