#include "pch.h"
#include "AdvanceAbilityTimelineSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<4> AdvanceAbilityTimelineSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<AdvanceAbilityTimelineSystem>(),
		"AdvanceAbilityTimelineSystem",
		std::array<AccessSpec, 4>
	{
		WriteImmediate(ComponentRes<AbilityStateComp>()),
		WriteImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	});

void AdvanceAbilityTimelineSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, abilityState, advance] :
		ctx.ecs.View<AbilityStateComp, AbilityTimelineAdvanceComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			!IsAbilityActive(abilityState))
		{
			continue;
		}

		const AbilityDef* abilityDef =
			GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId);
		if (abilityDef == nullptr)
		{
			continue;
		}

		const bool hasMatchingAdvance =
			advance.abilityId == abilityState.abilityId &&
			advance.abilityInstanceId == abilityState.abilityInstanceId;
		if (!hasMatchingAdvance)
		{
			continue;
		}

		abilityState.elapsedSec = advance.currElapsedSec;

		const float duration = std::max(0.001f, abilityDef->timeline.durationSec);
		for (const AbilityEventDef& eventDef : abilityDef->timeline.events)
		{
			const float eventTimeSec = eventDef.timeNormalized * duration;
			if (eventTimeSec < advance.prevElapsedSec ||
				eventTimeSec >= advance.currElapsedSec)
			{
				continue;
			}

			advance.events.push_back(eventDef);
		}
	}
}
