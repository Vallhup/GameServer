#include "pch.h"
#include "SpawnProjectileSystem.h"

#include "../../../CombatAreaProjectileDef.h"
#include "../../../GameplayContentCatalog.h"
#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const ProjectileDef* ResolveProjectileDef(
		const PendingProjectileSpawnRequest& request)
	{
		const GameplayContentCatalogSnapshot& catalog =
			GameplayContentCatalogSnapshot::Current();
		if (request.projectileId.has_value() &&
			*request.projectileId != InvalidProjectileId)
		{
			return catalog.Projectiles().Find(*request.projectileId);
		}
		if (request.projectileKey.has_value())
			return catalog.FindProjectileByKey(*request.projectileKey);
		return nullptr;
	}

	XMFLOAT3 BuildRight(const XMFLOAT3& direction)
	{
		return XMFLOAT3{ -direction.z, 0.0f, direction.x };
	}
}

const StaticSystemMetaStorage<4> SpawnProjectileSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<SpawnProjectileSystem>(),
		"SpawnProjectileSystem",
		std::array<AccessSpec, 4>
	{
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteDeferred(CommandBufferRes()),
		ReadImmediate(ExternalRes<ProjectileDef>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
	});

void SpawnProjectileSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, pending] : ctx.ecs.View<PendingProjectileSpawnComp>())
	{
		for (const PendingProjectileSpawnRequest& request : pending.requests)
		{
			if ((!request.projectileId.has_value() ||
					*request.projectileId == InvalidProjectileId) &&
				!request.projectileKey.has_value())
			{
				continue;
			}

			const ProjectileDef* def = ResolveProjectileDef(request);
			if (def == nullptr)
			{
				if (ReplicationStatsComp* stats =
					ctx.ecs.GetMutableComponent<ReplicationStatsComp>(entity))
				{
					++stats->droppedInvalidPayloadCount;
				}
				continue;
			}

			XMFLOAT3 direction = request.direction;
			if (!TransformHelper::TryNormalize(direction))
				direction = XMFLOAT3{ 0.0f, 0.0f, -1.0f };
			const XMFLOAT3 right = BuildRight(direction);

			WorldTransformComp transform{};
			transform.position = TransformHelper::Add(
				request.origin,
				TransformHelper::Add(
					TransformHelper::Scale(direction, def->spawnForwardOffset),
					TransformHelper::Add(
						TransformHelper::Scale(right, def->spawnRightOffset),
						XMFLOAT3{ 0.0f, def->spawnVerticalOffset, 0.0f })));

			const Entity projectileEntity = ctx.runtime.ReserveEntity();
			if (projectileEntity.IsNull())
				continue;

			ctx.runtime.DeferredAddComponent<WorldTransformComp>(
				projectileEntity,
				transform);
			ctx.runtime.DeferredAddComponent<ProjectileStateComp>(
				projectileEntity,
				ProjectileStateComp{
					.projectileId = def->id,
					.projectileKey = def->key,
					.owner = request.sourceEntity,
					.sourceAbilityId = request.sourceAbilityId,
					.sourceAbilityInstanceId = request.sourceAbilityInstanceId,
					.previousPosition = transform.position,
					.direction = direction,
					.speed = def->speed,
					.elapsedSec = 0.0f,
					.travelledDistance = 0.0f,
					.remainingPierceCount = def->pierceCount
				});
			ctx.runtime.DeferredAddComponent<ProjectileHitDedupStateComp>(
				projectileEntity,
				ProjectileHitDedupStateComp{});
		}
	}
}
