#pragma once

#include "GameplayComponentPrerequisites.h"

struct PendingProjectileSpawnRequest
{
	Entity sourceEntity{ Entity::Null() };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	std::optional<uint16_t> payloadId;
};

struct PendingProjectileSpawnComp : Component
{
	std::vector<PendingProjectileSpawnRequest> requests;
};

struct PendingAbilityPresentationEvent
{
	Entity sourceEntity{ Entity::Null() };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	AbilityEventKind eventKind{ AbilityEventKind::PlayCue };
	std::optional<uint16_t> payloadId;
};

struct PendingAbilityPresentationEventComp : Component
{
	std::vector<PendingAbilityPresentationEvent> events;
};
