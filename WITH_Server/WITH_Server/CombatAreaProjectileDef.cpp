#include "pch.h"
#include "CombatAreaProjectileDef.h"

#include "DefCompilePipeline.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"

#include <algorithm>
#include <filesystem>
#include <unordered_set>

using json = nlohmann::json;

namespace
{
	struct AreaShapeDto
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

	struct AreaScaleKeyDto
	{
		float timeSec{ 0.0f };
		float radiusScale{ 1.0f };
		float lengthScale{ 1.0f };
		float widthScale{ 1.0f };
	};

	struct AttackHitDto
	{
		float damageScale{ 0.0f };
		float bonusDamage{ 0.0f };
		float staminaDamageScale{ 0.0f };
		float bonusStaminaDamage{ 0.0f };
		float poiseDamageScale{ 0.0f };
		float bonusPoiseDamage{ 0.0f };
		float knockbackDistance{ 0.0f };
		float hitStopSec{ 0.0f };
		bool parryable{ false };
		bool guardable{ false };
	};

	struct AreaHitDto
	{
		std::string key;
		AreaShapeDto shape;
		AttackHitDto attackHit;
		std::vector<AreaScaleKeyDto> scaleKeys;
		float lifetimeSec{ 0.0f };
		float tickIntervalSec{ 0.0f };
		bool hitOncePerAbilityInstance{ true };
		bool includeOwner{ false };
		bool requireLineOfSight{ false };
	};

	struct ProjectileDto
	{
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
		AttackHitDto attackHit;
		std::optional<std::string> impactAreaHitKey;
	};

	template<typename TEnum, typename TParser>
	bool ReadTypedString(
		const json& node,
		const char* field,
		TEnum& outValue,
		const char* typeName,
		TParser parser,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!parser(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	bool ReadOptionalString(
		const json& node,
		const char* field,
		std::optional<std::string>& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at(field).is_string())
		{
			outError = std::string("Invalid string field: ") + field;
			return false;
		}

		outValue = node.at(field).get<std::string>();
		return true;
	}

	bool ReadOptionalBool(
		const json& node,
		const char* field,
		bool& outValue,
		std::string& outError)
	{
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_boolean())
		{
			outError = std::string("Invalid bool field: ") + field;
			return false;
		}

		outValue = node.at(field).get<bool>();
		return true;
	}

	bool ParseAreaShapeType(std::string_view text, AreaShapeType& outValue)
	{
		if (text == "Sphere") { outValue = AreaShapeType::Sphere; return true; }
		if (text == "Cylinder") { outValue = AreaShapeType::Cylinder; return true; }
		if (text == "Box") { outValue = AreaShapeType::Box; return true; }
		if (text == "Capsule") { outValue = AreaShapeType::Capsule; return true; }
		if (text == "Cone") { outValue = AreaShapeType::Cone; return true; }
		if (text == "Line") { outValue = AreaShapeType::Line; return true; }
		if (text == "Trapezoid") { outValue = AreaShapeType::Trapezoid; return true; }
		return false;
	}

	bool ParseAreaOriginPolicy(std::string_view text, AreaOriginPolicy& outValue)
	{
		if (text == "OwnerPosition") { outValue = AreaOriginPolicy::OwnerPosition; return true; }
		if (text == "OwnerForwardOffset") { outValue = AreaOriginPolicy::OwnerForwardOffset; return true; }
		if (text == "OwnerRightOffset") { outValue = AreaOriginPolicy::OwnerRightOffset; return true; }
		if (text == "LockedActionDirectionOffset") { outValue = AreaOriginPolicy::LockedActionDirectionOffset; return true; }
		if (text == "TargetPosition") { outValue = AreaOriginPolicy::TargetPosition; return true; }
		if (text == "ProjectileImpactPosition") { outValue = AreaOriginPolicy::ProjectileImpactPosition; return true; }
		if (text == "ExplicitWorldPosition") { outValue = AreaOriginPolicy::ExplicitWorldPosition; return true; }
		return false;
	}

	bool ParseProjectileMotionType(
		std::string_view text,
		ProjectileMotionType& outValue)
	{
		if (text == "Linear") { outValue = ProjectileMotionType::Linear; return true; }
		if (text == "Homing") { outValue = ProjectileMotionType::Homing; return true; }
		if (text == "Ballistic") { outValue = ProjectileMotionType::Ballistic; return true; }
		return false;
	}

	bool ParseProjectileCollisionShapeType(
		std::string_view text,
		ProjectileCollisionShapeType& outValue)
	{
		if (text == "Sphere") { outValue = ProjectileCollisionShapeType::Sphere; return true; }
		if (text == "Capsule") { outValue = ProjectileCollisionShapeType::Capsule; return true; }
		return false;
	}

	bool ParseProjectileHitPolicy(
		std::string_view text,
		ProjectileHitPolicy& outValue)
	{
		if (text == "DestroyOnFirstHit") { outValue = ProjectileHitPolicy::DestroyOnFirstHit; return true; }
		if (text == "PierceCount") { outValue = ProjectileHitPolicy::PierceCount; return true; }
		if (text == "HitOncePerTarget") { outValue = ProjectileHitPolicy::HitOncePerTarget; return true; }
		if (text == "ExplodeOnHit") { outValue = ProjectileHitPolicy::ExplodeOnHit; return true; }
		return false;
	}

	bool ParseProjectileSpawnDirectionPolicy(
		std::string_view text,
		ProjectileSpawnDirectionPolicy& outValue)
	{
		if (text == "OwnerFacing") { outValue = ProjectileSpawnDirectionPolicy::OwnerFacing; return true; }
		if (text == "LockedActionDirection") { outValue = ProjectileSpawnDirectionPolicy::LockedActionDirection; return true; }
		if (text == "TargetDirection") { outValue = ProjectileSpawnDirectionPolicy::TargetDirection; return true; }
		return false;
	}

	bool ParseAttackHit(
		const json& node,
		AttackHitDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredNumber(node, "damageScale", outDto.damageScale, outError) &&
			ReadRequiredNumber(node, "bonusDamage", outDto.bonusDamage, outError) &&
			ReadRequiredNumber(node, "staminaDamageScale", outDto.staminaDamageScale, outError) &&
			ReadRequiredNumber(node, "bonusStaminaDamage", outDto.bonusStaminaDamage, outError) &&
			ReadRequiredNumber(node, "poiseDamageScale", outDto.poiseDamageScale, outError) &&
			ReadRequiredNumber(node, "bonusPoiseDamage", outDto.bonusPoiseDamage, outError) &&
			ReadRequiredNumber(node, "knockbackDistance", outDto.knockbackDistance, outError) &&
			ReadRequiredNumber(node, "hitStopSec", outDto.hitStopSec, outError) &&
			ReadRequiredBool(node, "parryable", outDto.parryable, outError) &&
			ReadRequiredBool(node, "guardable", outDto.guardable, outError);
	}

	bool ParseAreaShape(
		const json& node,
		AreaShapeDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Area shape must be an object.";
			return false;
		}

		return
			ReadTypedString(node, "shapeType", outDto.shapeType, "AreaShapeType", ParseAreaShapeType, outError) &&
			ReadTypedString(node, "originPolicy", outDto.originPolicy, "AreaOriginPolicy", ParseAreaOriginPolicy, outError) &&
			ReadOptionalNumber(node, "radius", outDto.radius, outError) &&
			ReadOptionalNumber(node, "height", outDto.height, outError) &&
			ReadOptionalNumber(node, "length", outDto.length, outError) &&
			ReadOptionalNumber(node, "width", outDto.width, outError) &&
			ReadOptionalNumber(node, "halfAngleDeg", outDto.halfAngleDeg, outError) &&
			ReadOptionalNumber(node, "nearWidth", outDto.nearWidth, outError) &&
			ReadOptionalNumber(node, "farWidth", outDto.farWidth, outError) &&
			ReadOptionalNumber(node, "depth", outDto.depth, outError) &&
			ReadOptionalNumber(node, "forwardOffset", outDto.forwardOffset, outError) &&
			ReadOptionalNumber(node, "rightOffset", outDto.rightOffset, outError) &&
			ReadOptionalNumber(node, "verticalOffset", outDto.verticalOffset, outError) &&
			ReadOptionalNumber(node, "verticalTolerance", outDto.verticalTolerance, outError);
	}

	bool ParseScaleKey(
		const json& node,
		AreaScaleKeyDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredNumber(node, "timeSec", outDto.timeSec, outError) &&
			ReadOptionalNumber(node, "radiusScale", outDto.radiusScale, outError) &&
			ReadOptionalNumber(node, "lengthScale", outDto.lengthScale, outError) &&
			ReadOptionalNumber(node, "widthScale", outDto.widthScale, outError);
	}

	bool ParseAreaHit(const json& node, AreaHitDto& outDto, std::string& outError)
	{
		if (!ReadRequiredString(node, "key", outDto.key, outError) ||
			!ParseAreaShape(node.at("shape"), outDto.shape, outError) ||
			!ParseAttackHit(node.at("attackHit"), outDto.attackHit, outError) ||
			!ReadOptionalNumber(node, "lifetimeSec", outDto.lifetimeSec, outError) ||
			!ReadOptionalNumber(node, "tickIntervalSec", outDto.tickIntervalSec, outError) ||
			!ReadOptionalBool(node, "hitOncePerAbilityInstance", outDto.hitOncePerAbilityInstance, outError) ||
			!ReadOptionalBool(node, "includeOwner", outDto.includeOwner, outError) ||
			!ReadOptionalBool(node, "requireLineOfSight", outDto.requireLineOfSight, outError))
		{
			return false;
		}

		if (node.contains("scaleKeys") && !node.at("scaleKeys").is_null())
		{
			if (!node.at("scaleKeys").is_array())
			{
				outError = "Area scaleKeys must be an array.";
				return false;
			}

			for (const json& item : node.at("scaleKeys"))
			{
				AreaScaleKeyDto key{};
				if (!ParseScaleKey(item, key, outError))
					return false;
				outDto.scaleKeys.push_back(key);
			}
		}

		return true;
	}

	bool ParseProjectile(
		const json& node,
		ProjectileDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ReadTypedString(node, "motionType", outDto.motionType, "ProjectileMotionType", ParseProjectileMotionType, outError) &&
			ReadTypedString(node, "collisionShape", outDto.collisionShape, "ProjectileCollisionShapeType", ParseProjectileCollisionShapeType, outError) &&
			ReadTypedString(node, "hitPolicy", outDto.hitPolicy, "ProjectileHitPolicy", ParseProjectileHitPolicy, outError) &&
			ReadTypedString(node, "directionPolicy", outDto.directionPolicy, "ProjectileSpawnDirectionPolicy", ParseProjectileSpawnDirectionPolicy, outError) &&
			ReadRequiredNumber(node, "speed", outDto.speed, outError) &&
			ReadRequiredNumber(node, "maxLifetimeSec", outDto.maxLifetimeSec, outError) &&
			ReadRequiredNumber(node, "radius", outDto.radius, outError) &&
			ReadOptionalNumber(node, "length", outDto.length, outError) &&
			ReadOptionalNumber(node, "maxDistance", outDto.maxDistance, outError) &&
			ReadOptionalNumber(node, "spawnForwardOffset", outDto.spawnForwardOffset, outError) &&
			ReadOptionalNumber(node, "spawnRightOffset", outDto.spawnRightOffset, outError) &&
			ReadOptionalNumber(node, "spawnVerticalOffset", outDto.spawnVerticalOffset, outError) &&
			ReadOptionalNumber(node, "homingTurnRateDegPerSec", outDto.homingTurnRateDegPerSec, outError) &&
			ReadOptionalNumber(node, "pierceCount", outDto.pierceCount, outError) &&
			ParseAttackHit(node.at("attackHit"), outDto.attackHit, outError) &&
			ReadOptionalString(node, "impactAreaHitKey", outDto.impactAreaHitKey, outError);
	}

	bool AppendAreaHitDocumentDtos(
		const json& root,
		std::vector<AreaHitDto>& outDtos,
		std::string& outError)
	{
		const json* arrayNode = nullptr;
		if (root.is_array())
			arrayNode = &root;
		else if (root.contains("areaHits") && root.at("areaHits").is_array())
			arrayNode = &root.at("areaHits");
		else if (root.contains("areaHitDefs") && root.at("areaHitDefs").is_array())
			arrayNode = &root.at("areaHitDefs");

		if (arrayNode == nullptr)
		{
			outError = "Area hit document must be an array or contain areaHits.";
			return false;
		}

		for (const json& item : *arrayNode)
		{
			AreaHitDto dto{};
			if (!ParseAreaHit(item, dto, outError))
				return false;
			outDtos.push_back(std::move(dto));
		}
		return true;
	}

	bool AppendProjectileDocumentDtos(
		const json& root,
		std::vector<ProjectileDto>& outDtos,
		std::string& outError)
	{
		const json* arrayNode = nullptr;
		if (root.is_array())
			arrayNode = &root;
		else if (root.contains("projectiles") && root.at("projectiles").is_array())
			arrayNode = &root.at("projectiles");
		else if (root.contains("projectileDefs") && root.at("projectileDefs").is_array())
			arrayNode = &root.at("projectileDefs");

		if (arrayNode == nullptr)
		{
			outError = "Projectile document must be an array or contain projectiles.";
			return false;
		}

		for (const json& item : *arrayNode)
		{
			ProjectileDto dto{};
			if (!ParseProjectile(item, dto, outError))
				return false;
			outDtos.push_back(std::move(dto));
		}
		return true;
	}

	template<typename TDto, typename TId>
	bool BuildKeyIndex(
		std::span<const DefParsedDto<TDto>> dtos,
		TId firstRuntimeId,
		DefKeyIndex<TId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const DefParsedDto<TDto>& parsed : dtos)
			keys.push_back(parsed.dto.key);

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			firstRuntimeId,
			outIndex,
			outError);
	}

	AbilityAttackHitDef CompileAttackHit(const AttackHitDto& dto)
	{
		return AbilityAttackHitDef{
			.damageScale = dto.damageScale,
			.bonusDamage = dto.bonusDamage,
			.staminaDamageScale = dto.staminaDamageScale,
			.bonusStaminaDamage = dto.bonusStaminaDamage,
			.poiseDamageScale = dto.poiseDamageScale,
			.bonusPoiseDamage = dto.bonusPoiseDamage,
			.knockbackDistance = dto.knockbackDistance,
			.hitStopSec = dto.hitStopSec,
			.parryable = dto.parryable,
			.guardable = dto.guardable
		};
	}

	bool CompileAreaHit(
		const AreaHitDto& dto,
		const DefKeyIndex<AreaHitId>& keyIndex,
		AreaHitDef& outDef,
		std::string& outError)
	{
		if (!keyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown area hit key: " + dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.shape = AreaShapeDef{
			.shapeType = dto.shape.shapeType,
			.originPolicy = dto.shape.originPolicy,
			.radius = dto.shape.radius,
			.height = dto.shape.height,
			.length = dto.shape.length,
			.width = dto.shape.width,
			.halfAngleDeg = dto.shape.halfAngleDeg,
			.nearWidth = dto.shape.nearWidth,
			.farWidth = dto.shape.farWidth,
			.depth = dto.shape.depth,
			.forwardOffset = dto.shape.forwardOffset,
			.rightOffset = dto.shape.rightOffset,
			.verticalOffset = dto.shape.verticalOffset,
			.verticalTolerance = dto.shape.verticalTolerance
		};
		outDef.attackHit = CompileAttackHit(dto.attackHit);
		outDef.scaleKeys.clear();
		outDef.scaleKeys.reserve(dto.scaleKeys.size());
		for (const AreaScaleKeyDto& key : dto.scaleKeys)
		{
			outDef.scaleKeys.push_back(AreaScaleKeyDef{
				.timeSec = key.timeSec,
				.radiusScale = key.radiusScale,
				.lengthScale = key.lengthScale,
				.widthScale = key.widthScale
			});
		}
		outDef.lifetimeSec = dto.lifetimeSec;
		outDef.tickIntervalSec = dto.tickIntervalSec;
		outDef.hitOncePerAbilityInstance = dto.hitOncePerAbilityInstance;
		outDef.includeOwner = dto.includeOwner;
		outDef.requireLineOfSight = dto.requireLineOfSight;
		return true;
	}

	bool CompileProjectile(
		const ProjectileDto& dto,
		const DefKeyIndex<ProjectileId>& projectileKeyIndex,
		const DefKeyIndex<AreaHitId>& areaHitKeyIndex,
		ProjectileDef& outDef,
		std::string& outError)
	{
		if (!projectileKeyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown projectile key: " + dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.motionType = dto.motionType;
		outDef.collisionShape = dto.collisionShape;
		outDef.hitPolicy = dto.hitPolicy;
		outDef.directionPolicy = dto.directionPolicy;
		outDef.speed = dto.speed;
		outDef.maxLifetimeSec = dto.maxLifetimeSec;
		outDef.radius = dto.radius;
		outDef.length = dto.length;
		outDef.maxDistance = dto.maxDistance;
		outDef.spawnForwardOffset = dto.spawnForwardOffset;
		outDef.spawnRightOffset = dto.spawnRightOffset;
		outDef.spawnVerticalOffset = dto.spawnVerticalOffset;
		outDef.homingTurnRateDegPerSec = dto.homingTurnRateDegPerSec;
		outDef.pierceCount = dto.pierceCount;
		outDef.attackHit = CompileAttackHit(dto.attackHit);
		outDef.impactAreaHitId = std::nullopt;
		outDef.impactAreaHitKey = dto.impactAreaHitKey;
		if (dto.impactAreaHitKey.has_value())
		{
			AreaHitId areaHitId{ InvalidAreaHitId };
			if (!areaHitKeyIndex.FindId(*dto.impactAreaHitKey, areaHitId))
			{
				outError = "Unknown impact area hit key: " + *dto.impactAreaHitKey;
				return false;
			}
			outDef.impactAreaHitId = areaHitId;
		}
		return true;
	}

	DefLoadResult BuildEmptyAreaRegistry(AreaHitDefRegistry& outRegistry)
	{
		DefLoadResult result{};
		std::vector<AreaHitDef> defs;
		std::string error;
		if (!outRegistry.Build(std::move(defs), &error))
		{
			result.error = error;
			return result;
		}
		result.succeeded = true;
		return result;
	}

	DefLoadResult BuildEmptyProjectileRegistry(ProjectileDefRegistry& outRegistry)
	{
		DefLoadResult result{};
		std::vector<ProjectileDef> defs;
		std::string error;
		if (!outRegistry.Build(std::move(defs), &error))
		{
			result.error = error;
			return result;
		}
		result.succeeded = true;
		return result;
	}

	bool IsMissingOrEmptyDirectory(const std::filesystem::path& directory)
	{
		std::error_code ec;
		if (!std::filesystem::exists(directory, ec) ||
			!std::filesystem::is_directory(directory, ec))
		{
			return true;
		}

		for (const std::filesystem::directory_entry& entry :
			std::filesystem::recursive_directory_iterator(directory, ec))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
				return false;
		}
		return true;
	}

	bool BuildAreaHitKeyIndex(
		std::span<const AreaHitDef> areaHits,
		DefKeyIndex<AreaHitId>& outIndex,
		std::string& outError)
	{
		std::vector<DefKeyEntry<AreaHitId>> entries;
		entries.reserve(areaHits.size());
		for (const AreaHitDef& areaHit : areaHits)
		{
			entries.push_back(DefKeyEntry<AreaHitId>{
				.key = areaHit.key,
				.id = areaHit.id
			});
		}
		return outIndex.Build(std::move(entries), &outError);
	}
}

bool ValidateAreaHitDefs(
	std::span<const AreaHitDef> defs,
	std::string& outError)
{
	std::unordered_set<AreaHitId> ids;
	std::unordered_set<std::string> keys;
	for (const AreaHitDef& def : defs)
	{
		if (def.id == InvalidAreaHitId || def.key.empty())
		{
			outError = "AreaHit id and key must be valid.";
			return false;
		}
		if (!ids.insert(def.id).second || !keys.insert(def.key).second)
		{
			outError = "Duplicate AreaHit id or key.";
			return false;
		}

		const AreaShapeDef& shape = def.shape;
		switch (shape.shapeType)
		{
		case AreaShapeType::Sphere:
		case AreaShapeType::Cylinder:
			if (shape.radius <= 0.0f)
			{
				outError = "Sphere/Cylinder AreaHit requires radius > 0.";
				return false;
			}
			break;
		case AreaShapeType::Box:
			if (shape.width <= 0.0f || shape.length <= 0.0f)
			{
				outError = "Box AreaHit requires width and length > 0.";
				return false;
			}
			break;
		case AreaShapeType::Line:
			if (shape.length <= 0.0f || shape.radius <= 0.0f)
			{
				outError = "Line AreaHit requires length and radius > 0.";
				return false;
			}
			break;
		case AreaShapeType::Trapezoid:
			if (shape.nearWidth <= 0.0f || shape.farWidth <= 0.0f ||
				shape.depth <= 0.0f)
			{
				outError = "Trapezoid AreaHit requires nearWidth, farWidth, and depth > 0.";
				return false;
			}
			break;
		default:
			break;
		}

		float previousTime = -1.0f;
		for (const AreaScaleKeyDef& scaleKey : def.scaleKeys)
		{
			if (scaleKey.timeSec < previousTime)
			{
				outError = "AreaHit scale keys must be sorted by timeSec.";
				return false;
			}
			previousTime = scaleKey.timeSec;
		}
	}
	return true;
}

bool ValidateProjectileDefs(
	std::span<const ProjectileDef> defs,
	std::span<const AreaHitDef> areaHits,
	std::string& outError)
{
	std::unordered_set<ProjectileId> ids;
	std::unordered_set<std::string> keys;
	std::unordered_set<AreaHitId> areaIds;
	for (const AreaHitDef& areaHit : areaHits)
		areaIds.insert(areaHit.id);

	for (const ProjectileDef& def : defs)
	{
		if (def.id == InvalidProjectileId || def.key.empty())
		{
			outError = "Projectile id and key must be valid.";
			return false;
		}
		if (!ids.insert(def.id).second || !keys.insert(def.key).second)
		{
			outError = "Duplicate Projectile id or key.";
			return false;
		}
		if (def.motionType != ProjectileMotionType::Linear)
		{
			outError = "Only Linear projectile motion is currently supported.";
			return false;
		}
		if (def.speed <= 0.0f || def.radius <= 0.0f ||
			def.maxLifetimeSec <= 0.0f)
		{
			outError = "Projectile requires speed, radius, and maxLifetimeSec > 0.";
			return false;
		}
		if (def.maxDistance < 0.0f)
		{
			outError = "Projectile maxDistance must not be negative.";
			return false;
		}
		if (def.hitPolicy == ProjectileHitPolicy::PierceCount &&
			def.pierceCount < 1)
		{
			outError = "PierceCount projectile requires pierceCount >= 1.";
			return false;
		}
		if (def.hitPolicy == ProjectileHitPolicy::ExplodeOnHit &&
			(!def.impactAreaHitId.has_value() ||
				areaIds.find(*def.impactAreaHitId) == areaIds.end()))
		{
			outError = "ExplodeOnHit projectile requires a valid impactAreaHit.";
			return false;
		}
	}
	return true;
}

DefLoadResult LoadAreaHitDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AreaHitDefRegistry& outRegistry)
{
	if (IsMissingOrEmptyDirectory(directory))
		return BuildEmptyAreaRegistry(outRegistry);

	std::vector<DefJsonDocument> documents;
	DefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "AreaHit");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<AreaHitDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendAreaHitDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<AreaHitId> keyIndex;
	if (!BuildKeyIndex(
			std::span<const DefParsedDto<AreaHitDto>>(dtos.data(), dtos.size()),
			static_cast<AreaHitId>(InvalidAreaHitId + 1),
			keyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<AreaHitDef> defs;
	if (!CompileRuntimeDefs<AreaHitDto, AreaHitDef>(
			std::span<const DefParsedDto<AreaHitDto>>(dtos.data(), dtos.size()),
			[&keyIndex](
				const AreaHitDto& dto,
				AreaHitDef& outDef,
				std::string& outError)
			{
				return CompileAreaHit(dto, keyIndex, outDef, outError);
			},
			defs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::sort(defs.begin(), defs.end(), [](const AreaHitDef& lhs, const AreaHitDef& rhs) {
		return lhs.id < rhs.id;
	});

	if (!ValidateAreaHitDefs(
			std::span<const AreaHitDef>(defs.data(), defs.size()),
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	AreaHitDefRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}
	outRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = outRegistry.Size();
	result.error.clear();
	return result;
}

DefLoadResult LoadProjectileDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AreaHitDef> areaHits,
	ProjectileDefRegistry& outRegistry)
{
	if (IsMissingOrEmptyDirectory(directory))
		return BuildEmptyProjectileRegistry(outRegistry);

	std::vector<DefJsonDocument> documents;
	DefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "Projectile");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<ProjectileDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendProjectileDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<ProjectileId> projectileKeyIndex;
	if (!BuildKeyIndex(
			std::span<const DefParsedDto<ProjectileDto>>(dtos.data(), dtos.size()),
			static_cast<ProjectileId>(InvalidProjectileId + 1),
			projectileKeyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<AreaHitId> areaHitKeyIndex;
	if (!BuildAreaHitKeyIndex(areaHits, areaHitKeyIndex, result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<ProjectileDef> defs;
	if (!CompileRuntimeDefs<ProjectileDto, ProjectileDef>(
			std::span<const DefParsedDto<ProjectileDto>>(dtos.data(), dtos.size()),
			[&projectileKeyIndex, &areaHitKeyIndex](
				const ProjectileDto& dto,
				ProjectileDef& outDef,
				std::string& outError)
			{
				return CompileProjectile(
					dto,
					projectileKeyIndex,
					areaHitKeyIndex,
					outDef,
					outError);
			},
			defs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::sort(defs.begin(), defs.end(), [](const ProjectileDef& lhs, const ProjectileDef& rhs) {
		return lhs.id < rhs.id;
	});

	if (!ValidateProjectileDefs(
			std::span<const ProjectileDef>(defs.data(), defs.size()),
			areaHits,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	ProjectileDefRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}
	outRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = outRegistry.Size();
	result.error.clear();
	return result;
}
