#pragma once

#include "GameplayComponentPrerequisites.h"

struct PendingDespawnTag : TagComponent
{
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
