#pragma once

#include "GameplayComponentPrerequisites.h"

struct GimmickObjectComp : Component
{
	Entity ownerBoss{ Entity::Null() };
	Entity assignedPlayer{ Entity::Null() };
	Entity lastHitBy{ Entity::Null() };
	int32_t lastSyncedHp{ -1 };
	bool broken{ false };
};

struct StaticBoxHurtColliderComp : Component
{
	XMFLOAT3 halfExtents{ 0.75f, 1.0f, 0.75f };
};

struct SafeZoneComp : Component
{
	Entity ownerBoss{ Entity::Null() };
	float radius{ 1.25f };
	float remainingSec{ 0.0f };
};

struct BossGimmickImmunityComp : Component
{
	Entity ownerBoss{ Entity::Null() };
	float remainingSec{ 0.0f };
};

enum class BossGimmickObjectSyncState : uint32_t
{
	Spawned = 0,
	Updated = 1,
	Broken = 2,
	Despawned = 3
};

struct PendingBossGimmickObjectSyncEvent
{
	Entity boss{ Entity::Null() };
	uint32_t gimmickSeq{ 0 };
	uint64_t objectNetId{ 0 };
	BossGimmickObjectSyncState state{ BossGimmickObjectSyncState::Spawned };
	XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	float radius{ 0.0f };
	uint32_t curHp{ 0 };
	uint32_t maxHp{ 0 };
};

struct PendingBossGimmickZoneSyncEvent
{
	Entity boss{ Entity::Null() };
	uint32_t gimmickSeq{ 0 };
	uint64_t zoneNetId{ 0 };
	BossGimmickObjectSyncState state{ BossGimmickObjectSyncState::Spawned };
	XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	float radius{ 0.0f };
};

struct PendingBossGimmickReplicationComp : Component
{
	std::vector<PendingBossGimmickObjectSyncEvent> objectEvents;
	std::vector<PendingBossGimmickZoneSyncEvent> zoneEvents;
};
