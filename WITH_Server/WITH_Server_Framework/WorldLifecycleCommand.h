#pragma once

#include <cstdint>

#include "Entity.h"

enum class WorldLifecycleCommandKind : uint8_t
{
	RequestClose,
	CompletionReached,
	ConnectionAttached,
	ConnectionDetached,
	ConnectionDisconnected,
	EntitySpawned,
	EntityDespawned,
	TransferImported,
	TransferReleased
};

struct WorldLifecycleCommand
{
	WorldLifecycleCommandKind kind{ WorldLifecycleCommandKind::RequestClose };

	uint32_t connectionId{ 0 };
	Entity entity{};

	[[nodiscard]]
	static WorldLifecycleCommand RequestClose() noexcept
	{
		return { WorldLifecycleCommandKind::RequestClose };
	}

	[[nodiscard]]
	static WorldLifecycleCommand CompletionReached() noexcept
	{
		return { WorldLifecycleCommandKind::CompletionReached };
	}

	[[nodiscard]]
	static WorldLifecycleCommand ConnectionAttached(
		uint32_t connectionId,
		Entity entity = Entity{}) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::ConnectionAttached;
		cmd.connectionId = connectionId;
		cmd.entity = entity;
		return cmd;
	}

	[[nodiscard]]
	static WorldLifecycleCommand ConnectionDetached(
		uint32_t connectionId,
		Entity entity = Entity{}) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::ConnectionDetached;
		cmd.connectionId = connectionId;
		cmd.entity = entity;
		return cmd;
	}

	[[nodiscard]]
	static WorldLifecycleCommand ConnectionDisconnected(
		uint32_t connectionId,
		Entity entity = Entity{}) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::ConnectionDisconnected;
		cmd.connectionId = connectionId;
		cmd.entity = entity;
		return cmd;
	}

	[[nodiscard]]
	static WorldLifecycleCommand EntitySpawned(Entity entity) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::EntitySpawned;
		cmd.entity = entity;
		return cmd;
	}

	[[nodiscard]]
	static WorldLifecycleCommand EntityDespawned(Entity entity) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::EntityDespawned;
		cmd.entity = entity;
		return cmd;
	}

	[[nodiscard]]
	static WorldLifecycleCommand TransferImported(
		uint32_t connectionId,
		Entity entity = Entity{}) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::TransferImported;
		cmd.connectionId = connectionId;
		cmd.entity = entity;
		return cmd;
	}

	[[nodiscard]]
	static WorldLifecycleCommand TransferReleased(
		uint32_t connectionId,
		Entity entity = Entity{}) noexcept
	{
		WorldLifecycleCommand cmd;
		cmd.kind = WorldLifecycleCommandKind::TransferReleased;
		cmd.connectionId = connectionId;
		cmd.entity = entity;
		return cmd;
	}
};
