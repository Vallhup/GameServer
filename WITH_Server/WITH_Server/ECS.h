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

struct ActionRequestEvent {
	Entity entity;
	ActionType type;
};

struct MapCollisionEvent {
	Entity entity;
	uint32 obbId;
	XMFLOAT3 normal;
	float penetration;
};

struct OBB {
	uint32 id;

	XMFLOAT3 center;
	XMFLOAT3 axisX;
	XMFLOAT3 axisY;
	XMFLOAT3 axisZ;
	XMFLOAT3 extent;

	AABB aabb;
};

struct MapCollisionPair {
	Entity entity;
	uint32 obbId;
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

	std::vector<MapCollisionEvent> mapCollisionEvents;
	std::vector<CombatCollisionEvent> combatCollisionEvents;
	std::vector<ActionRequestEvent> actionRequestEvents;

	std::vector<MapCollisionPair> mapCollisionPairs;

	// TEMP
	std::vector<OBB> mapOBBs;
};