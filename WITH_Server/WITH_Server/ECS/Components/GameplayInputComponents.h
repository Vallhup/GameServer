#pragma once

#include "GameplayComponentPrerequisites.h"
#include "../../GameplayContentIds.h"

enum class PlayerAbilityInputType : uint8_t
{
	None = 0,
	LightAttack,
	HeavyAttack,
	Dodge,
	Parry
};

struct PlayerControlIdentityComp : Component
{
	NetId netId{ NetId::Invalid() };
	SessionId ownerSessionId{ 0 };
};

struct PlayerMoveInputState
{
	float inputX{ 0.0f };
	float inputZ{ 0.0f };
	float cameraYawRad{ 0.0f };
	bool wantsRun{ false };
	uint64_t lastUpdatedFrame{ 0 };
};

struct PlayerGuardInputState
{
	bool isPressed{ false };
	uint64_t lastUpdatedFrame{ 0 };
};

struct ActorAbilityInputEvent
{
	PlayerAbilityInputType type{ PlayerAbilityInputType::None };
	AbilityId directAbilityId{ InvalidAbilityId };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };
	uint64_t requestedFrame{ 0 };
};

struct ActorInputComp : Component
{
	PlayerMoveInputState move;
	PlayerGuardInputState guard;
	ActorAbilityInputEvent ability;
};
