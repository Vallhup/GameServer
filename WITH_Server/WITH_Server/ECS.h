#pragma once

#include "Entity.h"
#include "SystemManager.h"
#include "ComponentStorage.h"

enum class CollisionType : uint8 { Strike, Clash };

struct CollisionEvent {
	CollisionType type;

	Entity attacker;
	Entity victim;

	uint32 attackId;
	uint16 aIndex;
	uint16 bIndex;
};

struct ActionRequestEvent {
	Entity entity;
	ActionType type;
};

struct ECS {
	template<CompT T>
	ComponentStorage<T>& GetStorage()
	{
		static ComponentStorage<T> storage;
		return storage;
	}

	EntityManager entityMng;
	SystemManager systemMng;

	std::vector<CollisionEvent> collisionEvents;
	std::vector<ActionRequestEvent> actionRequestEvents;
};