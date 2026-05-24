#pragma once

#include "GameplayComponentPrerequisites.h"
#include "../../GameplayContentIds.h"

enum class PlayerAbilityInputType : uint8_t
{
	None = 0,
	LightAttack,
	HeavyAttack,
	Dodge,
	Parry,
	UseItem
};

struct PlayerControlIdentityComp : Component
{
	NetId netId{ NetId::Invalid() };
	SessionId ownerSessionId{ 0 };
};

struct PlayerNetworkTimingComp : Component
{
	uint32_t latestRttMs{ 0 };
	uint32_t smoothedRttMs{ 0 };
	uint32_t rttVarMs{ 0 };
	uint32_t arrivalJitterMs{ 0 };
	uint32_t estimatedOneWayMs{ 0 };
	uint32_t sentProbeCount{ 0 };
	uint32_t receivedProbeCount{ 0 };
	uint32_t rejectedProbeCount{ 0 };
	uint32_t inputArrivalSampleCount{ 0 };
	uint64_t lastUpdatedFrame{ 0 };
	bool initialized{ false };
	bool arrivalJitterInitialized{ false };
};

struct PlayerNetworkCompensationComp : Component
{
	float jitterBudgetMs{ 0.0f };
	float abilityInputBufferDurationSec{ 0.18f };
	float attackDriftTolerance01{ 0.25f };
	uint32_t inputDelayFrames{ 2 };
	uint32_t interpolationDelayFrames{ 2 };
	uint64_t lastUpdatedFrame{ 0 };
	bool initialized{ false };
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
	Entity target{ Entity::Null() };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };
	uint64_t requestedFrame{ 0 };
	AnimationId clientAnimId{ AnimationId::None };
	float clientNormalizedTime{ 0.0f };
	uint32_t clientAbilityInstanceId{ 0 };
	bool hasClientAnimationTiming{ false };
};

struct ActorAbilityInputBufferState
{
	ActorAbilityInputEvent event;
	float remainingSec{ 0.0f };
	bool hasEvent{ false };
};

struct ActorInputComp : Component
{
	PlayerMoveInputState move;
	PlayerGuardInputState guard;
	ActorAbilityInputEvent ability;
	ActorAbilityInputBufferState abilityBuffer;
};
