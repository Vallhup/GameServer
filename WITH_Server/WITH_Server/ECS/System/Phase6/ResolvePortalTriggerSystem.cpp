#include "pch.h"
#include "ResolvePortalTriggerSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 7> kResolvePortalTriggerAccesses{
		WriteImmediate(ComponentRes<PortalTriggerStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ExternalRes<PortalTriggerDef>()),
	};
}

const StaticSystemMetaStorage<7> ResolvePortalTriggerSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolvePortalTriggerSystem>(),
		"ResolvePortalTriggerSystem",
		std::array<AccessSpec, 7>
	{
		WriteImmediate(ComponentRes<PortalTriggerStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ExternalRes<PortalTriggerDef>()),
	});

void ResolvePortalTriggerSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, triggerState, transform, abilityState, identity] :
		ctx.ecs.View<
			PortalTriggerStateComp,
			WorldTransformComp,
			AbilityStateComp,
			PlayerControlIdentityComp>())
	{
		(void)transform;
		(void)identity;

		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			continue;
		}

		if (IsAbilityActive(abilityState))
		{
			continue;
		}

		// v0.1 fallback: no portal trigger resource/query API is wired yet.
		triggerState.activeTriggerId = 0;
		triggerState.wasInsideTrigger = false;
	}
}