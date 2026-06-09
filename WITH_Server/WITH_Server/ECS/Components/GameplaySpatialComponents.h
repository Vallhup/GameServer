#pragma once

#include "GameplayComponentPrerequisites.h"

struct WorldTransformComp : Component
{
	XMFLOAT3 position{ 161.352478f, 48.737797f, 644.831543f };
	XMFLOAT4 rotation{ 0, 0, 0, 1 };
	XMFLOAT3 scale{ 1, 1, 1 };
};

struct LocomotionMoveDeltaComp : Component
{
	XMFLOAT3 deltaPosition{ 0.0f, 0.0f, 0.0f };
	float deltaYawRad{ 0.0f };
	bool hasDelta{ false };
};

struct AbilityMoveDeltaComp : Component
{
	XMFLOAT3 deltaPosition{ 0.0f, 0.0f, 0.0f };
	float deltaYawRad{ 0.0f };
	bool hasDelta{ false };
};

struct AbilityMoveRuntimeComp : Component
{
	uint32_t boundAbilityInstanceId{ 0 };
	float lockedDirX{ 0.0f };
	float lockedDirZ{ 0.0f };
	float lockedYawRad{ 0.0f };
	bool hasLockedDirection{ false };

	float targetDashSegmentStartSec{ 0.0f };
	float targetDashStartX{ 0.0f };
	float targetDashStartZ{ 0.0f };
	float targetDashTargetX{ 0.0f };
	float targetDashTargetZ{ 0.0f };
	bool hasLockedTargetDash{ false };
};

struct PreCollisionTransformComp : Component
{
	XMFLOAT3 prevPosition{ 0.0f, 0.0f, 0.0f };
	XMFLOAT4 prevRotation{ 0.0f, 0.0f, 0.0f, 1.0f };

	XMFLOAT3 candidatePosition{ 0.0f, 0.0f, 0.0f };
	XMFLOAT4 candidateRotation{ 0.0f, 0.0f, 0.0f, 1.0f };

	bool movedThisFrame{ false };
	bool rotatedThisFrame{ false };
	bool preserveAbilityVerticalAboveNavMesh{ false };
};

struct BodyCollisionShapeComp : Component
{
	float bodyRadiusXZ{ 0.5f };
	float bodyHeight{ 1.8f };
	bool blocksBodyOverlap{ true };
	bool useNavMeshConstraint{ true };
	BodyPushability pushability{ BodyPushability::Dynamic };
	float overlapYieldWeight{ 1.0f };
	float maxOverlapCorrectionPerFrameXZ{ 0.12f };
};

struct NavMeshAgentStateComp : Component
{
	uint64_t currentPolyRef{ 0 };
};

struct BodyCollisionResolveComp : Component
{
	XMFLOAT3 navResolvedPosition{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 collisionSlideDelta{ 0.0f, 0.0f, 0.0f };
	bool navMeshAdjusted{ false };
	bool navMeshFallbackNoProvider{ false };
	bool terrainHeightAdjusted{ false };
	bool terrainHeightFallbackNoProvider{ false };
	bool terrainHeightSampleFailed{ false };
	bool rejectedByNavMesh{ false };
	bool overlapAdjusted{ false };
	bool slideAdjusted{ false };
};

struct PortalTriggerStateComp : Component
{
	uint16_t activeTriggerId{ 0 };
	bool wasInsideTrigger{ false };
};

struct PortalTriggerDef
{
	uint16_t triggerId{ 0 };
	XMFLOAT3 center{ 0.0f, 0.0f, 0.0f };
	float radiusXZ{ 0.0f };
	WorldDefId targetWorldDefId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };
	SpawnPointId spawnPointId{ 0 };
	bool hasSpawnPointId{ false };
	bool allowFallback{ false };
};
