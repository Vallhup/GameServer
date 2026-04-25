#include "pch.h"
#include "ResolvePortalTriggerSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 7> kResolvePortalTriggerAccesses{
		WriteImmediate(ComponentRes<PortalTriggerStateComp>()),
		ReadSnapshot(ComponentRes<WorldTransformComp>()),
		ReadSnapshot(ComponentRes<ActionStateComp>()),
		ReadSnapshot(ComponentRes<PlayerControlIdentityComp>()),
		ReadSnapshot(ComponentRes<PendingDespawnTag>()),
		ReadSnapshot(ComponentRes<PendingWorldTransferTag>()),
		ReadSnapshot(ExternalRes<PortalTriggerDef>()),
	};
}

const SystemMeta ResolvePortalTriggerSystem::kMeta =
	SystemMeta{
		SysTag<ResolvePortalTriggerSystem>(),
		"ResolvePortalTriggerSystem",
		kResolvePortalTriggerAccesses,
		kNoDeps,
		kNoDeps
	};

void ResolvePortalTriggerSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, triggerState, transform, actionState, identity] :
		ctx.ecs.View<
			PortalTriggerStateComp,
			WorldTransformComp,
			ActionStateComp,
			PlayerControlIdentityComp>())
	{
		(void)transform;
		(void)identity;

		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			continue;
		}

		if (IsActionActive(actionState))
		{
			continue;
		}

		// v0.1 fallback: no portal trigger resource/query API is wired yet.
		triggerState.activeTriggerId = 0;
		triggerState.wasInsideTrigger = false;
	}
}

const SystemMeta& ResolvePortalTriggerSystem::Meta() const
{
	return kMeta;
}
