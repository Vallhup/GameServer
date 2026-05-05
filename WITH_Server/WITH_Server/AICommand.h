#pragma once

#include <cstdint>

#include "GameplayContentIds.h"
#include "WorldCommand.h"

enum class AICommandTypeKey : WorldCommandTypeKey
{
	None = 0,
	Move = 100,
	Ability = 101
};

struct AIMoveCommandPayload
{
	float inputX{ 0.0f };
	float inputZ{ 0.0f };
	float yaw{ 0.0f };
	uint8_t isRun{ 0 };
};

struct AIAbilityCommandPayload
{
	AbilityId abilityId{ InvalidAbilityId };
	float dirX{ 0.0f };
	float dirZ{ 0.0f };
};

bool IsAIMoveCommandType(WorldCommandTypeKey typeKey) noexcept;
bool IsAIAbilityCommandType(WorldCommandTypeKey typeKey) noexcept;
