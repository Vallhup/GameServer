#include "pch.h"
#include "SpawnAreaVolumeSystem.h"

#include "../../../CombatAreaProjectileDef.h"
#include "../../../GameplayContentCatalog.h"
#include "../GameplaySystemUtil.h"

namespace
{
	const AreaHitDef* ResolveAreaHitDef(const PendingAreaHitRequest& request)
	{
		const GameplayContentCatalogSnapshot& catalog =
			GameplayContentCatalogSnapshot::Current();
		if (request.areaHitId != InvalidAreaHitId)
			return catalog.AreaHits().Find(request.areaHitId);
		if (request.areaHitKey.has_value())
			return catalog.FindAreaHitByKey(*request.areaHitKey);
		return nullptr;
	}
}

const StaticSystemMetaStorage<4> SpawnAreaVolumeSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<SpawnAreaVolumeSystem>(),
		"SpawnAreaVolumeSystem",
		std::array<AccessSpec, 4>
	{
		ReadImmediate(ComponentRes<PendingAreaHitComp>()),
		WriteDeferred(CommandBufferRes()),
		ReadImmediate(ExternalRes<AreaHitDef>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
	});

void SpawnAreaVolumeSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, pending] : ctx.ecs.View<PendingAreaHitComp>())
	{
		for (const PendingAreaHitRequest& request : pending.requests)
		{
			if (!request.spawnVolume)
				continue;
			if (request.areaHitId == InvalidAreaHitId &&
				!request.areaHitKey.has_value())
			{
				continue;
			}

			const AreaHitDef* def = ResolveAreaHitDef(request);
			if (def == nullptr || def->lifetimeSec <= 0.0f)
			{
				if (ReplicationStatsComp* stats =
					ctx.ecs.GetMutableComponent<ReplicationStatsComp>(entity))
				{
					++stats->droppedInvalidPayloadCount;
				}
				continue;
			}

			const Entity volumeEntity = ctx.runtime.ReserveEntity();
			if (volumeEntity.IsNull())
				continue;

			WorldTransformComp transform{};
			transform.position = request.origin;
			ctx.runtime.DeferredAddComponent<WorldTransformComp>(
				volumeEntity,
				transform);
			ctx.runtime.DeferredAddComponent<AreaVolumeStateComp>(
				volumeEntity,
				AreaVolumeStateComp{
					.owner = request.sourceEntity,
					.sourceAbilityId = request.sourceAbilityId,
					.sourceAbilityInstanceId =
						request.sourceAbilityInstanceId,
					.areaHitId = def->id,
					.areaHitKey = def->key,
					.origin = request.origin,
					.direction = request.direction,
					.elapsedSec = 0.0f,
					.lifetimeSec = def->lifetimeSec,
					.tickIntervalSec =
						def->tickIntervalSec > 0.0f
							? def->tickIntervalSec
							: 0.25f,
					.nextTickSec = 0.0f
				});
			ctx.runtime.DeferredAddComponent<AreaHitDedupStateComp>(
				volumeEntity,
				AreaHitDedupStateComp{});
		}
	}
}
