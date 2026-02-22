#pragma once

#include "AnimationType.h"
#include "Component.h"

struct PrebakedAnimation;

struct AnimationState : public Component {
	AnimationType desiredId{ AnimationType::Knight_Idle };
	double speed{ 1.0f };
	bool looping{ true };
};

struct Animator : public Component {
	const PrebakedAnimation* clip{ nullptr };
	uint16 currentFrame{ 0 };
};

struct LocomotionAnimPhase : public Component {
	double phase{ 0.0f };
	bool wasMoving{ false };
};