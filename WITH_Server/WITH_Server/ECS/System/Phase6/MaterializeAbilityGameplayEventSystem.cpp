#include "pch.h"
#include "MaterializeAbilityGameplayEventSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	XMFLOAT3 MakeForward(const WorldTransformComp& transform)
	{
		const XMVECTOR forward = TransformHelper::Forward(transform);
		return XMFLOAT3{
			XMVectorGetX(forward),
			0.0f,
			XMVectorGetZ(forward)
		};
	}

	XMFLOAT3 MakeDirection(
		const AbilityStateComp& abilityState,
		const WorldTransformComp& transform)
	{
		float dirX = abilityState.directionX;
		float dirZ = abilityState.directionZ;
		if (TransformHelper::NormalizeXZ(dirX, dirZ))
			return XMFLOAT3{ dirX, 0.0f, dirZ };
		return MakeForward(transform);
	}
}

const StaticSystemMetaStorage<9> MaterializeAbilityGameplayEventSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<MaterializeAbilityGameplayEventSystem>(),
		"MaterializeAbilityGameplayEventSystem",
		std::array<AccessSpec, 9>
	{
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<PendingCombatResultComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteImmediate(ComponentRes<PendingAreaHitComp>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
	});

void MaterializeAbilityGameplayEventSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, advance, abilityState, transform] :
		ctx.ecs.View<
			AbilityTimelineAdvanceComp,
			AbilityStateComp,
			WorldTransformComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			advance.abilityId != abilityState.abilityId ||
			advance.abilityInstanceId != abilityState.abilityInstanceId)
		{
			continue;
		}

		for (uint16_t eventIndex = 0;
			eventIndex < static_cast<uint16_t>(advance.events.size());
			++eventIndex)
		{
			const AbilityEventDef& eventDef = advance.events[eventIndex];
			if (eventDef.condition == AbilityEventTriggerCondition::OnParrySuccess)
			{
				const PendingCombatResultComp* result =
					ctx.ecs.GetComponent<PendingCombatResultComp>(entity);
				if (result == nullptr || !result->parrySucceededThisFrame)
					continue;
			}

			if (eventDef.kind == AbilityEventKind::SpawnProjectile)
			{
				PendingProjectileSpawnComp* projectile =
					ctx.ecs.GetMutableComponent<PendingProjectileSpawnComp>(entity);
				if (projectile == nullptr)
					continue;

				projectile->requests.push_back(PendingProjectileSpawnRequest{
					.sourceEntity = entity,
					.sourceAbilityId = advance.abilityId,
					.sourceAbilityInstanceId = advance.abilityInstanceId,
					.payloadId = eventDef.payloadId,
					.projectileId = eventDef.projectileId,
					.projectileKey = eventDef.projectileKey,
					.origin = transform.position,
					.direction = MakeDirection(abilityState, transform)
				});
			}
			else if (
				eventDef.kind == AbilityEventKind::TriggerAreaHit ||
				eventDef.kind == AbilityEventKind::SpawnAreaVolume)
			{
				PendingAreaHitComp* area =
					ctx.ecs.GetMutableComponent<PendingAreaHitComp>(entity);
				if (area == nullptr)
					continue;

				area->requests.push_back(PendingAreaHitRequest{
					.sourceEntity = entity,
					.sourceProxyEntity = Entity::Null(),
					.sourceAbilityId = advance.abilityId,
					.sourceAbilityInstanceId = advance.abilityInstanceId,
					.sourceEventIndex = eventIndex,
					.areaHitId = eventDef.areaHitId.value_or(InvalidAreaHitId),
					.areaHitKey = eventDef.areaHitKey,
					.origin = transform.position,
					.direction = MakeDirection(abilityState, transform),
					.elapsedSec = 0.0f,
					.spawnVolume =
						eventDef.kind == AbilityEventKind::SpawnAreaVolume
				});
			}
		}
	}
}
