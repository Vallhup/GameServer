#include "pch.h"
#include "ResolvePortalTriggerSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolvePortalTriggerSystem::kMeta =
	MakeSystemMeta<ResolvePortalTriggerSystem>("ResolvePortalTriggerSystem");

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
