#include "pch.h"
#include "ResolveAreaHitSystem.h"

#include "../../../CombatAreaProjectileDef.h"
#include "../../../GameplayContentCatalog.h"
#include "../GameplaySystemUtil.h"
#include "CombatHitResolutionUtil.h"

using namespace GameplaySystemUtil;
using namespace CombatHitResolutionUtil;

namespace
{
	struct AreaRuntimeShape
	{
		AreaShapeDef shape;
		XMFLOAT3 origin{ 0.0f, 0.0f, 0.0f };
		XMFLOAT3 direction{ 0.0f, 0.0f, -1.0f };
		XMFLOAT3 right{ 1.0f, 0.0f, 0.0f };
	};

	const AreaHitDef* ResolveAreaHitDef(
		const AreaHitId areaHitId,
		const std::optional<std::string>& areaHitKey)
	{
		const GameplayContentCatalogSnapshot& catalog =
			GameplayContentCatalogSnapshot::Current();
		if (areaHitId != InvalidAreaHitId)
			return catalog.AreaHits().Find(areaHitId);
		if (areaHitKey.has_value())
			return catalog.FindAreaHitByKey(*areaHitKey);
		return nullptr;
	}

	XMFLOAT3 BuildRight(const XMFLOAT3& direction)
	{
		return XMFLOAT3{ -direction.z, 0.0f, direction.x };
	}

	float LerpFloat(float from, float to, float alpha) noexcept
	{
		return from + (to - from) * alpha;
	}

	AreaScaleKeyDef ResolveScaleKey(
		const std::vector<AreaScaleKeyDef>& scaleKeys,
		float elapsedSec)
	{
		if (elapsedSec <= scaleKeys.front().timeSec)
			return scaleKeys.front();

		for (size_t i = 1; i < scaleKeys.size(); ++i)
		{
			const AreaScaleKeyDef& prev = scaleKeys[i - 1];
			const AreaScaleKeyDef& next = scaleKeys[i];
			if (elapsedSec > next.timeSec)
				continue;

			const float spanSec = next.timeSec - prev.timeSec;
			if (spanSec <= 1.0e-5f)
				return next;

			const float alpha = std::clamp(
				(elapsedSec - prev.timeSec) / spanSec,
				0.0f,
				1.0f);

			return AreaScaleKeyDef{
				.timeSec = elapsedSec,
				.radiusScale = LerpFloat(
					prev.radiusScale,
					next.radiusScale,
					alpha),
				.lengthScale = LerpFloat(
					prev.lengthScale,
					next.lengthScale,
					alpha),
				.widthScale = LerpFloat(
					prev.widthScale,
					next.widthScale,
					alpha)
			};
		}

		return scaleKeys.back();
	}

	void ApplyScaleKeys(
		const AreaHitDef& def,
		float elapsedSec,
		AreaShapeDef& shape)
	{
		if (def.scaleKeys.empty())
			return;

		const AreaScaleKeyDef key =
			ResolveScaleKey(def.scaleKeys, elapsedSec);

		shape.radius *= key.radiusScale;
		shape.length *= key.lengthScale;
		shape.width *= key.widthScale;
		shape.nearWidth *= key.widthScale;
		shape.farWidth *= key.widthScale;
		shape.depth *= key.lengthScale;
	}

	AreaRuntimeShape BuildRuntimeShape(
		const AreaHitDef& def,
		const XMFLOAT3& baseOrigin,
		const XMFLOAT3& baseDirection,
		float elapsedSec)
	{
		AreaRuntimeShape result{};
		result.shape = def.shape;
		ApplyScaleKeys(def, elapsedSec, result.shape);

		result.direction = baseDirection;
		if (!TransformHelper::TryNormalize(result.direction))
			result.direction = XMFLOAT3{ 0.0f, 0.0f, -1.0f };
		result.right = BuildRight(result.direction);

		result.origin = TransformHelper::Add(
			baseOrigin,
			TransformHelper::Add(
				TransformHelper::Scale(
					result.direction,
					result.shape.forwardOffset),
				TransformHelper::Add(
					TransformHelper::Scale(result.right, result.shape.rightOffset),
					XMFLOAT3{ 0.0f, result.shape.verticalOffset, 0.0f })));
		return result;
	}

	bool PassesVertical(
		const AreaRuntimeShape& area,
		const XMFLOAT3& point) noexcept
	{
		const float tolerance = area.shape.verticalTolerance;
		if (tolerance <= 0.0f)
			return true;
		return std::abs(point.y - area.origin.y) <= tolerance;
	}

	bool OverlapsArea(
		const AreaRuntimeShape& area,
		const Capsule& targetCapsule,
		float targetRadius,
		XMFLOAT3& outImpactPoint)
	{
		const XMFLOAT3 center = CapsuleCenter(targetCapsule);
		outImpactPoint = center;

		switch (area.shape.shapeType)
		{
		case AreaShapeType::Sphere:
		{
			const Capsule pointCapsule{ area.origin, area.origin };
			return CapsulesOverlap(
				pointCapsule,
				area.shape.radius,
				targetCapsule,
				targetRadius);
		}
		case AreaShapeType::Cylinder:
		{
			if (!PassesVertical(area, center))
				return false;
			const float dx = center.x - area.origin.x;
			const float dz = center.z - area.origin.z;
			const float radius = area.shape.radius + targetRadius;
			return dx * dx + dz * dz <= radius * radius;
		}
		case AreaShapeType::Line:
		{
			const Capsule lineCapsule{
				area.origin,
				TransformHelper::Add(
					area.origin,
					TransformHelper::Scale(area.direction, area.shape.length))
			};
			return CapsulesOverlap(
				lineCapsule,
				area.shape.radius,
				targetCapsule,
				targetRadius);
		}
		case AreaShapeType::Capsule:
		{
			const Capsule areaCapsule{
				area.origin,
				TransformHelper::Add(
					area.origin,
					TransformHelper::Scale(area.direction, area.shape.length))
			};
			return CapsulesOverlap(
				areaCapsule,
				area.shape.radius,
				targetCapsule,
				targetRadius);
		}
		case AreaShapeType::Box:
		{
			if (!PassesVertical(area, center))
				return false;
			const XMFLOAT3 toCenter = TransformHelper::Subtract(center, area.origin);
			const float localX = TransformHelper::DotF(toCenter, area.right);
			const float localZ = TransformHelper::DotF(toCenter, area.direction);
			const float halfWidth = area.shape.width * 0.5f + targetRadius;
			const float halfLength = area.shape.length * 0.5f + targetRadius;
			return std::abs(localX) <= halfWidth &&
				std::abs(localZ) <= halfLength;
		}
		case AreaShapeType::Trapezoid:
		{
			if (!PassesVertical(area, center))
				return false;
			const XMFLOAT3 toCenter = TransformHelper::Subtract(center, area.origin);
			const float localX = TransformHelper::DotF(toCenter, area.right);
			const float localZ = TransformHelper::DotF(toCenter, area.direction);
			const float depth = area.shape.depth;
			if (localZ < -targetRadius || localZ > depth + targetRadius)
				return false;
			const float alpha = std::clamp(localZ / std::max(depth, 1.0e-4f), 0.0f, 1.0f);
			const float width =
				area.shape.nearWidth +
				(area.shape.farWidth - area.shape.nearWidth) * alpha;
			return std::abs(localX) <= width * 0.5f + targetRadius;
		}
		case AreaShapeType::Cone:
		{
			if (!PassesVertical(area, center))
				return false;
			XMFLOAT3 toCenter = TransformHelper::Subtract(center, area.origin);
			toCenter.y = 0.0f;
			const float distSq = TransformHelper::DotF(toCenter, toCenter);
			const float radius = area.shape.radius + targetRadius;
			if (distSq > radius * radius)
				return false;
			if (!TransformHelper::TryNormalize(toCenter))
				return true;
			const float dot = TransformHelper::DotF(area.direction, toCenter);
			const float cosThreshold =
				std::cos(area.shape.halfAngleDeg * (XM_PI / 180.0f));
			return dot >= cosThreshold;
		}
		default:
			return false;
		}
	}

	bool IsDeduped(const AreaHitDedupStateComp* dedup, Entity victim)
	{
		return dedup != nullptr &&
			std::find(
				dedup->resolvedVictims.begin(),
				dedup->resolvedVictims.end(),
				victim) != dedup->resolvedVictims.end();
	}

	void AddDedup(AreaHitDedupStateComp* dedup, Entity victim)
	{
		if (dedup != nullptr)
			dedup->resolvedVictims.push_back(victim);
	}

	void ResolveOneArea(
		SystemContext& ctx,
		Entity source,
		Entity sourceProxy,
		AreaHitDedupStateComp* dedup,
		const AreaHitDef& def,
		AbilityId abilityId,
		uint32_t abilityInstanceId,
		uint16_t eventIndex,
		const XMFLOAT3& origin,
		const XMFLOAT3& direction,
		float elapsedSec)
	{
		AreaRuntimeShape area = BuildRuntimeShape(def, origin, direction, elapsedSec);

		for (auto [victim, victimAbility, victimTransform, victimColliders, victimActivation] :
			ctx.ecs.View<
				AbilityStateComp,
				WorldTransformComp,
				SkeletalCombatColliderComp,
				CombatColliderActivationComp>())
		{
			if ((!def.includeOwner && victim == source) ||
				HasBlockingPendingState(ctx.ecs, victim) ||
				ShouldBlockSameFactionCombat(
					ctx.runtime,
					ctx.ecs,
					source,
					victim) ||
				victimActivation.hasInvulnerabilityWindow ||
				(def.hitOncePerAbilityInstance && IsDeduped(dedup, victim)))
			{
				continue;
			}

			PendingCombatInteractionRecord interaction{};
			const bool hit = TryBuildNonSkeletalInteractionRecord(
				ctx.ecs,
				source,
				sourceProxy,
				CombatHitSourceKind::AreaHit,
				abilityId,
				abilityInstanceId,
				eventIndex,
				def.attackHit,
				area.origin,
				area.direction,
				victim,
				victimAbility,
				victimTransform,
				victimColliders,
				victimActivation,
				[&area](
					const Capsule& targetCapsule,
					float targetRadius,
					CombatResolveResultType,
					XMFLOAT3& impactPoint)
				{
					return OverlapsArea(
						area,
						targetCapsule,
						targetRadius,
						impactPoint);
				},
				interaction);
			if (!hit)
				continue;

			AppendInteractionResult(ctx.ecs, source, victim, interaction);
			if (def.hitOncePerAbilityInstance)
				AddDedup(dedup, victim);
		}
	}
}

const StaticSystemMetaStorage<13> ResolveAreaHitSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveAreaHitSystem>(),
		"ResolveAreaHitSystem",
		std::array<AccessSpec, 13>
	{
		WriteImmediate(ComponentRes<PendingAreaHitComp>()),
		WriteImmediate(ComponentRes<AreaVolumeStateComp>()),
		WriteImmediate(ComponentRes<AreaHitDedupStateComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<SkeletalCombatColliderComp>()),
		ReadImmediate(ComponentRes<CombatColliderActivationComp>()),
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<PendingCombatImpactEventComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteDeferred(CommandBufferRes()),
	});

void ResolveAreaHitSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, pending] : ctx.ecs.View<PendingAreaHitComp>())
	{
		AreaHitDedupStateComp* dedup =
			ctx.ecs.GetMutableComponent<AreaHitDedupStateComp>(entity);
		for (const PendingAreaHitRequest& request : pending.requests)
		{
			if (request.spawnVolume)
				continue;
			if (request.areaHitId == InvalidAreaHitId &&
				!request.areaHitKey.has_value())
			{
				continue;
			}

			const AreaHitDef* def =
				ResolveAreaHitDef(request.areaHitId, request.areaHitKey);
			if (def == nullptr)
				continue;

			if (dedup != nullptr &&
				dedup->boundSourceInstanceId != request.sourceAbilityInstanceId)
			{
				dedup->boundSourceInstanceId = request.sourceAbilityInstanceId;
				dedup->resolvedVictims.clear();
			}

			ResolveOneArea(
				ctx,
				request.sourceEntity,
				request.sourceProxyEntity,
				dedup,
				*def,
				request.sourceAbilityId,
				request.sourceAbilityInstanceId,
				request.sourceEventIndex,
				request.origin,
				request.direction,
				request.elapsedSec);
		}

		pending.requests.clear();
	}

	const float dtSec = static_cast<float>(std::max(0.0, ctx.dtSec));
	for (auto [volumeEntity, volume, dedup] :
		ctx.ecs.View<AreaVolumeStateComp, AreaHitDedupStateComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, volumeEntity))
			continue;

		const AreaHitDef* def =
			ResolveAreaHitDef(volume.areaHitId, volume.areaHitKey);
		if (def == nullptr || HasBlockingPendingState(ctx.ecs, volume.owner))
		{
			ctx.runtime.DeferredDestroyEntityIfAlive(volumeEntity);
			continue;
		}

		volume.elapsedSec += dtSec;
		if (volume.elapsedSec + 1.0e-4f >= volume.nextTickSec)
		{
			if (dedup.boundSourceInstanceId != volume.sourceAbilityInstanceId)
			{
				dedup.boundSourceInstanceId = volume.sourceAbilityInstanceId;
				dedup.resolvedVictims.clear();
			}

			ResolveOneArea(
				ctx,
				volume.owner,
				volumeEntity,
				&dedup,
				*def,
				volume.sourceAbilityId,
				volume.sourceAbilityInstanceId,
				0,
				volume.origin,
				volume.direction,
				volume.elapsedSec);
			volume.nextTickSec += std::max(0.001f, volume.tickIntervalSec);
		}

		if (volume.lifetimeSec > 0.0f && volume.elapsedSec >= volume.lifetimeSec)
			ctx.runtime.DeferredDestroyEntityIfAlive(volumeEntity);
	}
}
