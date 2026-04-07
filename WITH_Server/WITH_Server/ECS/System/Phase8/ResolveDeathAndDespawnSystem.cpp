#include "pch.h"
#include "ResolveDeathAndDespawnSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveDeathAndDespawnSystem::kMeta =
	MakeSystemMeta<ResolveDeathAndDespawnSystem>(
		"ResolveDeathAndDespawnSystem");

void ResolveDeathAndDespawnSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, stats] : ctx.ecs.View<CombatStatStateComp>())
	{
		if (stats.currentHp > 0)
		{
			continue;
		}

		if (!ctx.ecs.HasComponent<PendingDespawnTag>(entity))
		{
			ctx.runtime.DeferredAddComponent<PendingDespawnTag>(
				entity,
				PendingDespawnTag{});
		}
		if (ctx.ecs.HasComponent<PendingWorldTransferTag>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PendingWorldTransferTag>(entity);
		}
		if (ctx.ecs.HasComponent<PendingWorldTransferComp>(entity))
		{
			ctx.runtime.DeferredRemoveComponent<PendingWorldTransferComp>(entity);
		}
	}
}

const SystemMeta& ResolveDeathAndDespawnSystem::Meta() const
{
	return kMeta;
}
