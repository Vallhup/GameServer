#pragma once

#include "GameplayComponentPrerequisites.h"

struct PendingProjectileSpawnRequest
{
	Entity sourceEntity{ Entity::Null() };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
	std::optional<EventPayloadId> payloadId;
};

struct PendingProjectileSpawnComp : Component
{
	std::vector<PendingProjectileSpawnRequest> requests;
};

struct PendingActionPresentationEvent
{
	Entity sourceEntity{ Entity::Null() };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
	EventType eventType{ EventType::PlayEffect };
	std::optional<EventPayloadId> payloadId;
};

struct PendingActionPresentationEventComp : Component
{
	std::vector<PendingActionPresentationEvent> events;
};
