#include "pch.h"
#include "AdvanceProjectileSystem.h"

#include "../../../CombatAreaProjectileDef.h"
#include "../../../GameplayContentCatalog.h"
#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

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
}

const StaticSystemMetaStorage<5> AdvanceProjectileSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<AdvanceProjectileSystem>(),
		"AdvanceProjectileSystem",
		std::array<AccessSpec, 5>
	{
		WriteImmediate(ComponentRes<ProjectileStateComp>()),
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		WriteDeferred(CommandBufferRes()),
	});

void AdvanceProjectileSystem::Execute(SystemContext& ctx)
{
	const float dtSec = static_cast<float>(std::max(0.0, ctx.dtSec));
	if (dtSec <= 0.0f)
		return;

	for (auto [entity, state, transform] :
		ctx.ecs.View<ProjectileStateComp, WorldTransformComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity))
			continue;

		const ProjectileDef* def = ResolveProjectileDef(state);
		if (def == nullptr)
		{
			ctx.runtime.DeferredDestroyEntityIfAlive(entity);
			continue;
		}

		state.previousPosition = transform.position;
		const XMFLOAT3 delta = TransformHelper::Scale(state.direction, state.speed * dtSec);
		transform.position = TransformHelper::Add(transform.position, delta);
		state.travelledDistance += std::sqrt(TransformHelper::DotF(delta, delta));
		state.elapsedSec += dtSec;

		const bool expiredByTime =
			def->maxLifetimeSec > 0.0f &&
			state.elapsedSec >= def->maxLifetimeSec;
		const bool expiredByDistance =
			def->maxDistance > 0.0f &&
			state.travelledDistance >= def->maxDistance;
		if (expiredByTime || expiredByDistance)
			ctx.runtime.DeferredDestroyEntityIfAlive(entity);
	}
}
