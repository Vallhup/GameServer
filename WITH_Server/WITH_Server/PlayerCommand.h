#pragma once

#include <cstdint>

#include "Session.h"
#include "WorldCommand.h"

enum class PlayerCommandTypeKey : WorldCommandTypeKey
{
	None = 0,
	Move = 1,
	LightAttack = 2,
	HeavyAttack = 3,
	Dodge = 4,
	Guard = 5,
	Parry = 6,
	UseItem = 7
};

struct PlayerMoveCommandPayload
{
	float inputX{ 0.0f };
	float inputZ{ 0.0f };
	float yaw{ 0.0f };
	uint8_t isRun{ 0 };
};

struct PlayerDirectionCommandPayload
{
	float dirX{ 0.0f };
	float dirZ{ 0.0f };
};

struct PlayerGuardCommandPayload
{
	uint8_t pressed{ 0 };
};

bool IsPlayerMoveCommandType(WorldCommandTypeKey typeKey) noexcept;
bool IsPlayerAbilityEventCommandType(WorldCommandTypeKey typeKey) noexcept;
bool IsPlayerGuardCommandType(WorldCommandTypeKey typeKey) noexcept;

WorldCommand MakePlayerMoveCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerMoveCommandPayload& payload);

WorldCommand MakePlayerLightAttackCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload);

WorldCommand MakePlayerHeavyAttackCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload);

WorldCommand MakePlayerDodgeCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload);

WorldCommand MakePlayerGuardCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerGuardCommandPayload& payload);

WorldCommand MakePlayerParryCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload);

WorldCommand MakePlayerUseItemCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload);

bool TryDecodePlayerMoveCommandPayload(
	const WorldCommand& command,
	PlayerMoveCommandPayload& outPayload) noexcept;

bool TryDecodePlayerDirectionCommandPayload(
	const WorldCommand& command,
	PlayerDirectionCommandPayload& outPayload) noexcept;

bool TryDecodePlayerGuardCommandPayload(
	const WorldCommand& command,
	PlayerGuardCommandPayload& outPayload) noexcept;
