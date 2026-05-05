#pragma once

#include "GameplayContentComponents.h"
#include "GameplayComponentPrerequisites.h"

struct AbilityInterruptEvent
{
	AbilityTransitionCause cause{
		AbilityTransitionCause::OnHitReceived };
	Entity instigator{ Entity::Null() };
	uint64_t frameIndex{ 0 };
	int priority{ 0 };
};

struct AbilityInterruptQueueComp : Component
{
	std::vector<AbilityInterruptEvent> events;
};

enum class LocomotionMode : uint8_t
{
	Idle = 0,
	Walk,
	Run,
	Turn,
	WalkBack,
	WalkLeft,
	WalkRight,
	TurnLeft,
	TurnRight
};

struct LocomotionStateComp : Component
{
	LocomotionMode mode{ LocomotionMode::Idle };
	float desiredMoveDirX{ 0.0f };
	float desiredMoveDirZ{ 0.0f };
	float desiredFacingYawRad{ 0.0f };
	float facingYawRad{ 0.0f };
	float currentSpeed{ 0.0f };
	float locomotionAnimPhase01{ 0.0f };
	bool wasLocomotionMoving{ false };
};
