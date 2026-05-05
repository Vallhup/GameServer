#include "pch.h"
#include "PlayerCommand.h"

#include <cstring>
#include <type_traits>

namespace
{
	template<typename T>
	std::vector<std::byte> EncodePayload(const T& payload)
	{
		static_assert(std::is_trivially_copyable_v<T>);

		std::vector<std::byte> bytes(sizeof(T));
		std::memcpy(bytes.data(), &payload, sizeof(T));
		return bytes;
	}

	template<typename T>
	bool TryDecodePayload(
		const WorldCommand& command,
		T& outPayload) noexcept
	{
		static_assert(std::is_trivially_copyable_v<T>);

		if (command.payload.size() != sizeof(T))
		{
			return false;
		}

		std::memcpy(&outPayload, command.payload.data(), sizeof(T));
		return true;
	}

	WorldCommand MakePlayerCommand(
		SessionId sourceSessionId,
		NetId targetNetId,
		PlayerCommandTypeKey typeKey,
		uint32_t sequence,
		std::vector<std::byte> payload)
	{
		WorldCommand command{};
		command.sourceId = static_cast<WorldCommandSourceId>(sourceSessionId);
		command.targetNetId = targetNetId;
		command.typeKey = static_cast<WorldCommandTypeKey>(typeKey);
		command.sequence = sequence;
		command.payload = std::move(payload);
		return command;
	}
}

bool IsPlayerMoveCommandType(WorldCommandTypeKey typeKey) noexcept
{
	return typeKey == static_cast<WorldCommandTypeKey>(
		PlayerCommandTypeKey::Move);
}

bool IsPlayerAbilityEventCommandType(WorldCommandTypeKey typeKey) noexcept
{
	return typeKey == static_cast<WorldCommandTypeKey>(
		PlayerCommandTypeKey::LightAttack) ||
		typeKey == static_cast<WorldCommandTypeKey>(
			PlayerCommandTypeKey::HeavyAttack) ||
		typeKey == static_cast<WorldCommandTypeKey>(
			PlayerCommandTypeKey::Dodge) ||
		typeKey == static_cast<WorldCommandTypeKey>(
			PlayerCommandTypeKey::Parry);
}

bool IsPlayerGuardCommandType(WorldCommandTypeKey typeKey) noexcept
{
	return typeKey == static_cast<WorldCommandTypeKey>(
		PlayerCommandTypeKey::Guard);
}

WorldCommand MakePlayerMoveCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerMoveCommandPayload& payload)
{
	return MakePlayerCommand(
		sourceSessionId,
		targetNetId,
		PlayerCommandTypeKey::Move,
		sequence,
		EncodePayload(payload));
}

WorldCommand MakePlayerLightAttackCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload)
{
	return MakePlayerCommand(
		sourceSessionId,
		targetNetId,
		PlayerCommandTypeKey::LightAttack,
		sequence,
		EncodePayload(payload));
}

WorldCommand MakePlayerHeavyAttackCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload)
{
	return MakePlayerCommand(
		sourceSessionId,
		targetNetId,
		PlayerCommandTypeKey::HeavyAttack,
		sequence,
		EncodePayload(payload));
}

WorldCommand MakePlayerDodgeCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload)
{
	return MakePlayerCommand(
		sourceSessionId,
		targetNetId,
		PlayerCommandTypeKey::Dodge,
		sequence,
		EncodePayload(payload));
}

WorldCommand MakePlayerGuardCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerGuardCommandPayload& payload)
{
	return MakePlayerCommand(
		sourceSessionId,
		targetNetId,
		PlayerCommandTypeKey::Guard,
		sequence,
		EncodePayload(payload));
}

WorldCommand MakePlayerParryCommand(
	SessionId sourceSessionId,
	NetId targetNetId,
	uint32_t sequence,
	const PlayerDirectionCommandPayload& payload)
{
	return MakePlayerCommand(
		sourceSessionId,
		targetNetId,
		PlayerCommandTypeKey::Parry,
		sequence,
		EncodePayload(payload));
}

bool TryDecodePlayerMoveCommandPayload(
	const WorldCommand& command,
	PlayerMoveCommandPayload& outPayload) noexcept
{
	if (!IsPlayerMoveCommandType(command.typeKey))
	{
		return false;
	}

	return TryDecodePayload(command, outPayload);
}

bool TryDecodePlayerDirectionCommandPayload(
	const WorldCommand& command,
	PlayerDirectionCommandPayload& outPayload) noexcept
{
	if (!IsPlayerAbilityEventCommandType(command.typeKey))
	{
		return false;
	}

	return TryDecodePayload(command, outPayload);
}

bool TryDecodePlayerGuardCommandPayload(
	const WorldCommand& command,
	PlayerGuardCommandPayload& outPayload) noexcept
{
	if (!IsPlayerGuardCommandType(command.typeKey))
	{
		return false;
	}

	return TryDecodePayload(command, outPayload);
}
