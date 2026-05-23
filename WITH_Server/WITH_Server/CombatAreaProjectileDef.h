#pragma once

#include "AbilityDef.h"
#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "GameplayContentIds.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

enum class AreaShapeType : uint8_t
{
	Sphere,
	Cylinder,
	Box,
	Capsule,
	Cone,
	Line,
	Trapezoid
};

enum class AreaOriginPolicy : uint8_t
{
	OwnerPosition,
	OwnerForwardOffset,
	OwnerRightOffset,
	LockedActionDirectionOffset,
	TargetPosition,
	ProjectileImpactPosition,
	ExplicitWorldPosition
};

struct AreaShapeDef
{
	AreaShapeType shapeType{ AreaShapeType::Sphere };
	AreaOriginPolicy originPolicy{ AreaOriginPolicy::OwnerPosition };
	float radius{ 0.0f };
	float height{ 0.0f };
	float length{ 0.0f };
	float width{ 0.0f };
	float halfAngleDeg{ 0.0f };
	float nearWidth{ 0.0f };
	float farWidth{ 0.0f };
	float depth{ 0.0f };
	float forwardOffset{ 0.0f };
	float rightOffset{ 0.0f };
	float verticalOffset{ 0.0f };
	float verticalTolerance{ 0.0f };
};

struct AreaScaleKeyDef
{
	float timeSec{ 0.0f };
	float radiusScale{ 1.0f };
	float lengthScale{ 1.0f };
	float widthScale{ 1.0f };
};

struct AreaHitDef
{
	AreaHitId id{ InvalidAreaHitId };
	std::string key;
	AreaShapeDef shape;
	AbilityAttackHitDef attackHit;
	std::vector<AreaScaleKeyDef> scaleKeys;
	float lifetimeSec{ 0.0f };
	float tickIntervalSec{ 0.0f };
	bool hitOncePerAbilityInstance{ true };
	bool includeOwner{ false };
	bool requireLineOfSight{ false };
};

enum class ProjectileMotionType : uint8_t
{
	Linear,
	Homing,
	Ballistic
};

enum class ProjectileCollisionShapeType : uint8_t
{
	Sphere,
	Capsule
};

enum class ProjectileHitPolicy : uint8_t
{
	DestroyOnFirstHit,
	PierceCount,
	HitOncePerTarget,
	ExplodeOnHit
};

enum class ProjectileSpawnDirectionPolicy : uint8_t
{
	OwnerFacing,
	LockedActionDirection,
	TargetDirection
};

struct ProjectileDef
{
	ProjectileId id{ InvalidProjectileId };
	std::string key;
	ProjectileMotionType motionType{ ProjectileMotionType::Linear };
	ProjectileCollisionShapeType collisionShape{
		ProjectileCollisionShapeType::Sphere };
	ProjectileHitPolicy hitPolicy{ ProjectileHitPolicy::DestroyOnFirstHit };
	ProjectileSpawnDirectionPolicy directionPolicy{
		ProjectileSpawnDirectionPolicy::LockedActionDirection };
	float speed{ 0.0f };
	float maxLifetimeSec{ 0.0f };
	float radius{ 0.0f };
	float length{ 0.0f };
	float maxDistance{ 0.0f };
	float spawnForwardOffset{ 0.0f };
	float spawnRightOffset{ 0.0f };
	float spawnVerticalOffset{ 0.0f };
	float homingTurnRateDegPerSec{ 0.0f };
	int pierceCount{ 0 };
	AbilityAttackHitDef attackHit;
	std::optional<AreaHitId> impactAreaHitId;
	std::optional<std::string> impactAreaHitKey;
};

struct AreaHitDefTraits
{
	static AreaHitId GetId(const AreaHitDef& def) noexcept
	{
		return def.id;
	}
};

struct ProjectileDefTraits
{
	static ProjectileId GetId(const ProjectileDef& def) noexcept
	{
		return def.id;
	}
};

using AreaHitDefRegistry = DefRegistry<
	AreaHitDef,
	AreaHitId,
	AreaHitDefTraits>;

using ProjectileDefRegistry = DefRegistry<
	ProjectileDef,
	ProjectileId,
	ProjectileDefTraits>;

DefLoadResult LoadAreaHitDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AreaHitDefRegistry& outRegistry);

DefLoadResult LoadProjectileDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AreaHitDef> areaHits,
	ProjectileDefRegistry& outRegistry);

bool ValidateAreaHitDefs(
	std::span<const AreaHitDef> defs,
	std::string& outError);

bool ValidateProjectileDefs(
	std::span<const ProjectileDef> defs,
	std::span<const AreaHitDef> areaHits,
	std::string& outError);
