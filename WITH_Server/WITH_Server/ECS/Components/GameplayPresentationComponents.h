#pragma once

#include "GameplayComponentPrerequisites.h"

struct PendingProjectileSpawnRequest
{
	Entity sourceEntity{ Entity::Null() };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	std::optional<uint16_t> payloadId;
	std::optional<ProjectileId> projectileId;
	std::optional<std::string> projectileKey;
	XMFLOAT3 origin{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 direction{ 0.0f, 0.0f, -1.0f };
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
