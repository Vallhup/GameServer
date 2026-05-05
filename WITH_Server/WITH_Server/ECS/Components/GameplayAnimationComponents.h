#pragma once

#include "GameplayAbilityComponents.h"

enum class AnimationPlaybackSource : uint8_t
{
	None = 0,
	Locomotion,
	Ability
};

struct AnimationPlaybackStateComp : Component
{
	AnimationPlaybackSource source{ AnimationPlaybackSource::None };
	AnimationId animationId{ AnimationId::None };
	uint32_t boundAbilityInstanceId{ 0 };
	AbilityId boundAbilityId{ InvalidAbilityId };
	LocomotionMode boundLocomotionMode{ LocomotionMode::Idle };
	float playbackTimeSec{ 0.0f };
	float normalizedTime{ 0.0f };
	float playRate{ 1.0f };
	bool loop{ false };
	bool holdLastFrame{ false };
};

struct SampledAnimationPoseComp : Component
{
	AnimationId animationId{ AnimationId::None };
	uint16_t sampleFrameIndex{ 0 };
	std::vector<Capsule> localCapsules;
};
