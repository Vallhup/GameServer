#pragma once

#include "GameplayComponentPrerequisites.h"

struct PendingDespawnTag : TagComponent
{
};

struct PlayerDeathCountConsumedTag : TagComponent
{
};

struct PendingPlayerDeathCountEventComp : Component
{
	bool pending{ false };
};

enum class PlayerDeathState : uint8_t
{
	Alive = 0,
	WaitingForDeathCount,
	AwaitingRespawnInput,
	DeathCountExhausted
};

struct PlayerDeathStateComp : Component
{
	PlayerDeathState state{ PlayerDeathState::Alive };
	uint64_t deathCountRevision{ 0 };
	bool respawnRequested{ false };
};

struct PendingWorldTransferTag : TagComponent
{
};

struct PendingWorldTransferComp : Component
{
	WorldDefId targetWorldDefId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };
	SpawnPointId spawnPointId{ 0 };
	bool hasSpawnPointId{ false };
	bool allowFallback{ false };
	uint16_t sourceTriggerId{ 0 };
	uint64_t requestedFrameIndex{ 0 };
};
