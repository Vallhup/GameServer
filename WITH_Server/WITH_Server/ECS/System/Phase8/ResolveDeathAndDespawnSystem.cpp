#include "pch.h"
#include "ResolveDeathAndDespawnSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 6> kResolveDeathAndDespawnAccesses{
		ReadImmediate(ComponentRes<CombatStatStateComp>()),
		ReadImmediate(ComponentRes<ActionStateComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferComp>()),
		WriteDeferred(CommandBufferRes()),
	};
}

const SystemMeta ResolveDeathAndDespawnSystem::kMeta =
	SystemMeta{
		SysTag<ResolveDeathAndDespawnSystem>(),
		"ResolveDeathAndDespawnSystem",
		kResolveDeathAndDespawnAccesses,
		kNoDeps,
		kNoDeps,
		true,
		false
	};

void ResolveDeathAndDespawnSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, stats, actionState] :
		ctx.ecs.View<CombatStatStateComp, ActionStateComp>())
	{
		if (stats.currentHp > 0)
		{
			continue;
		}

		const ActionDef* actionDef =
			IsActionActive(actionState)
			? FindActionDef(actionState.actionId)
			: nullptr;
		const bool deadActionFinished =
			actionDef != nullptr &&
			actionDef->kind == ActionKind::Dead &&
			actionState.elapsedSec >= actionDef->duration;

		if (deadActionFinished &&
			!ctx.ecs.HasComponent<PendingDespawnTag>(entity))
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
