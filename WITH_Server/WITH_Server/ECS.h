#pragma once

#include "Entity.h"
#include "SystemManager.h"
#include "ComponentStorage.h"

enum class CollisionType : uint8 { Strike, Clash };

struct CombatCollisionEvent {
	CollisionType type;

	Entity attacker;
	Entity victim;

	uint32 attackId;
	uint16 aIndex;
	uint16 bIndex;
};

enum class ActionRequestReason : uint8 {
	None,
	FromCombat,
	FromInput,
	FromAI
};

struct ActionRequestEvent {
	Entity entity;
	ActionType actionType;
	AttackType attackType;
	ActionRequestReason reason;
};

struct DBEvent {

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

	std::vector<CombatCollisionEvent> combatCollisionEvents;
	std::vector<ActionRequestEvent> actionRequestEvents;

	concurrency::concurrent_queue<DBEvent> dbEvents;
};