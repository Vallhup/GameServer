#pragma once

#include "GameplayComponentPrerequisites.h"

struct GimmickObjectComp : Component
{
	Entity ownerBoss{ Entity::Null() };
	Entity assignedPlayer{ Entity::Null() };
	Entity lastHitBy{ Entity::Null() };
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
