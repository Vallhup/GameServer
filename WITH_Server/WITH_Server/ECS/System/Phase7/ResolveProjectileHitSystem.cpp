#include "pch.h"
#include "ResolveProjectileHitSystem.h"

#include "../../../CombatAreaProjectileDef.h"
#include "../../../GameplayContentCatalog.h"
#include "../GameplaySystemUtil.h"
#include "CombatHitResolutionUtil.h"

using namespace GameplaySystemUtil;
using namespace CombatHitResolutionUtil;

namespace
{
	const ProjectileDef* ResolveProjectileDef(const ProjectileStateComp& state)
	{
		const GameplayContentCatalogSnapshot& catalog =
			GameplayContentCatalogSnapshot::Current();
		if (state.projectileId != InvalidProjectileId)
			return catalog.Projectiles().Find(state.projectileId);
		if (state.projectileKey.has_value())
			return catalog.FindProjectileByKey(*state.projectileKey);
		return nullptr;
	}

	bool IsDeduped(const ProjectileHitDedupStateComp& dedup, Entity victim)
	{
		return std::find(
			dedup.resolvedVictims.begin(),
			dedup.resolvedVictims.end(),
			victim) != dedup.resolvedVictims.end();
	}

	void QueueImpactArea(
		SystemContext& ctx,
		const ProjectileStateComp& projectile,
		const ProjectileDef& def,
		const XMFLOAT3& impactPoint)
	{
		if (!def.impactAreaHitId.has_value())
			return;

		PendingAreaHitComp* area =
			ctx.ecs.GetMutableComponent<PendingAreaHitComp>(projectile.owner);
		if (area == nullptr)
			return;

		area->requests.push_back(PendingAreaHitRequest{
			.sourceEntity = projectile.owner,
			.sourceProxyEntity = Entity::Null(),
			.sourceAbilityId = projectile.sourceAbilityId,
			.sourceAbilityInstanceId = projectile.sourceAbilityInstanceId,
			.sourceEventIndex = 0,
			.areaHitId = *def.impactAreaHitId,
			.areaHitKey = def.impactAreaHitKey,
			.origin = impactPoint,
			.direction = projectile.direction,
			.elapsedSec = 0.0f,
			.spawnVolume = false
		});
	}
}

const StaticSystemMetaStorage<13> ResolveProjectileHitSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveProjectileHitSystem>(),
		"ResolveProjectileHitSystem",
		std::array<AccessSpec, 13>
	{
		ReadImmediate(ComponentRes<ProjectileStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<ProjectileHitDedupStateComp>()),
		WriteImmediate(ComponentRes<PendingAreaHitComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<SkeletalCombatColliderComp>()),
		ReadImmediate(ComponentRes<CombatColliderActivationComp>()),
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<PendingCombatImpactEventComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteDeferred(CommandBufferRes()),
	});

void ResolveProjectileHitSystem::Execute(SystemContext& ctx)
{
	std::vector<Entity> projectiles;
	for (auto [entity, state] : ctx.ecs.View<ProjectileStateComp>())
	{
		if (!HasBlockingPendingState(ctx.ecs, entity))
			projectiles.push_back(entity);
	}
	std::sort(projectiles.begin(), projectiles.end(), [](Entity lhs, Entity rhs) {
		return lhs.id < rhs.id;
	});

	for (Entity projectileEntity : projectiles)
	{
		ProjectileStateComp* projectile =
			ctx.ecs.GetMutableComponent<ProjectileStateComp>(projectileEntity);
		WorldTransformComp* projectileTransform =
			ctx.ecs.GetMutableComponent<WorldTransformComp>(projectileEntity);
		ProjectileHitDedupStateComp* dedup =
			ctx.ecs.GetMutableComponent<ProjectileHitDedupStateComp>(projectileEntity);
		if (projectile == nullptr ||
			projectileTransform == nullptr ||
			dedup == nullptr ||
			projectile->owner.IsNull() ||
			HasBlockingPendingState(ctx.ecs, projectile->owner))
		{
			continue;
		}

		const ProjectileDef* def = ResolveProjectileDef(*projectile);
		if (def == nullptr)
			continue;

		const Capsule sweepCapsule{
			projectile->previousPosition,
			projectileTransform->position
		};
		bool shouldDespawn = false;

		for (auto [victim, victimAbility, victimTransform, victimColliders, victimActivation] :
			ctx.ecs.View<
				AbilityStateComp,
				WorldTransformComp,
				SkeletalCombatColliderComp,
				CombatColliderActivationComp>())
		{
			if (victim == projectile->owner ||
				victim == projectileEntity ||
				HasBlockingPendingState(ctx.ecs, victim) ||
				IsSameFaction(ctx.ecs, projectile->owner, victim) ||
				victimActivation.hasInvulnerabilityWindow ||
				IsDeduped(*dedup, victim))
			{
				continue;
			}

			PendingCombatInteractionRecord interaction{};
			const bool hit = TryBuildNonSkeletalInteractionRecord(
				ctx.ecs,
				projectile->owner,
				projectileEntity,
				CombatHitSourceKind::Projectile,
				projectile->sourceAbilityId,
				projectile->sourceAbilityInstanceId,
				0,
				def->attackHit,
				projectileTransform->position,
				projectile->direction,
				victim,
				victimAbility,
				victimTransform,
				victimColliders,
				victimActivation,
				[&sweepCapsule, def](
					const Capsule& targetCapsule,
					float targetRadius,
					CombatResolveResultType,
					XMFLOAT3& impactPoint)
				{
					const SegmentClosestResult closest =
						ComputeSegmentClosestPoints(sweepCapsule, targetCapsule);
					const float sumRadius = def->radius + targetRadius;
					if (closest.distanceSq > sumRadius * sumRadius)
						return false;

					impactPoint = TransformHelper::Scale(
						TransformHelper::Add(
							closest.lhsPoint,
							closest.rhsPoint),
						0.5f);
					return true;
				},
				interaction);
			if (!hit)
				continue;

			AppendInteractionResult(
				ctx.ecs,
				projectile->owner,
				victim,
				interaction);
			dedup->resolvedVictims.push_back(victim);

			if (def->hitPolicy == ProjectileHitPolicy::ExplodeOnHit)
			{
				QueueImpactArea(ctx, *projectile, *def, interaction.impactPoint);
				shouldDespawn = true;
			}
			else if (def->hitPolicy == ProjectileHitPolicy::DestroyOnFirstHit)
			{
				shouldDespawn = true;
			}
			else if (def->hitPolicy == ProjectileHitPolicy::PierceCount)
			{
				--projectile->remainingPierceCount;
				if (projectile->remainingPierceCount <= 0)
					shouldDespawn = true;
			}

			if (shouldDespawn)
				break;
		}

		if (shouldDespawn)
			ctx.runtime.DeferredDestroyEntityIfAlive(projectileEntity);
	}
}
