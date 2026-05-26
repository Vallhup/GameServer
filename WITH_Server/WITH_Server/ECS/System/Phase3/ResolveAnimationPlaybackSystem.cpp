#include "pch.h"
#include "ResolveAnimationPlaybackSystem.h"

#include "../GameplaySystemUtil.h"
#include "../Phase2/ResolveAbilityStateSystem.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<6, 0, 1> ResolveAnimationPlaybackSystem::kMetaStorage =
    MakeMetaStorage(
        SysTag<ResolveAnimationPlaybackSystem>(),
        "ResolveAnimationPlaybackSystem",
        std::array<AccessSpec, 6>
        {
            WriteImmediate(ComponentRes<AnimationPlaybackStateComp>()),
            ReadImmediate(ComponentRes<AbilityStateComp>()),
            ReadImmediate(ComponentRes<LocomotionStateComp>()),
            ReadImmediate(ComponentRes<SpawnTypeComp>()),
            ReadImmediate(ComponentRes<CombatStatStateComp>()),
            WriteImmediate(ComponentRes<DirtyFlagsComp>()),
        },
        std::array<SystemTag, 0>{},
        std::array<SystemTag, 1>{ SysTag<ResolveAbilityStateSystem>() });

void ResolveAnimationPlaybackSystem::Execute(SystemContext& ctx)
{
	for (auto [
		entity,
		playbackState,
		abilityState,
		locomotionState,
		spawnType,
		stats] :
		ctx.ecs.View<
			AnimationPlaybackStateComp,
			AbilityStateComp,
			LocomotionStateComp,
			SpawnTypeComp,
			CombatStatStateComp>())
	{
		const AnimationPlaybackStateComp previousState = playbackState;

		const AbilityDef* activeAbilityDef =
			IsAbilityActive(abilityState)
			? GameplayContentCatalogSnapshot::Current()
				.Abilities()
				.Find(abilityState.abilityId)
			: nullptr;
		const bool activeAbilityIsDead =
			activeAbilityDef != nullptr &&
			activeAbilityDef->kind == AbilityKind::Dead;

		if (!activeAbilityIsDead &&
			stats.currentHp <= 0 &&
			previousState.source == AnimationPlaybackSource::Ability &&
			previousState.boundAbilityId != InvalidAbilityId)
		{
			const AbilityDef* previousAbilityDef =
				GameplayContentCatalogSnapshot::Current()
					.Abilities()
					.Find(previousState.boundAbilityId);

			if (previousAbilityDef != nullptr &&
				previousAbilityDef->kind == AbilityKind::Dead)
			{
				playbackState = previousState;
				playbackState.normalizedTime = 1.0f;
				playbackState.playbackTimeSec =
					previousAbilityDef->timeline.durationSec;
				playbackState.loop = false;
				playbackState.holdLastFrame = true;
				continue;
			}
		}

		if (IsAbilityActive(abilityState))
		{
			playbackState.source = AnimationPlaybackSource::Ability;
			playbackState.animationId =
				ResolveAbilityAnimationId(
					spawnType.characterId,
					abilityState.abilityId);
			playbackState.boundAbilityInstanceId = abilityState.abilityInstanceId;
			playbackState.boundAbilityId = abilityState.abilityId;
			playbackState.boundLocomotionMode = LocomotionMode::Idle;
			playbackState.playbackTimeSec = abilityState.elapsedSec;
			playbackState.normalizedTime =
				(activeAbilityDef != nullptr && activeAbilityDef->timeline.durationSec > 0.0f)
				? ClampFloat(abilityState.elapsedSec / activeAbilityDef->timeline.durationSec, 0.0f, 1.0f)
				: 0.0f;
			playbackState.playRate = 1.0f;
			playbackState.loop = false;
			playbackState.holdLastFrame = true;
		}
		else
		{
			playbackState.source = AnimationPlaybackSource::Locomotion;
			playbackState.animationId =
				ResolveLocomotionAnimationId(
					spawnType.characterId,
					locomotionState.mode);
			playbackState.boundAbilityInstanceId = 0;
			playbackState.boundAbilityId = InvalidAbilityId;
			playbackState.boundLocomotionMode = locomotionState.mode;
			playbackState.normalizedTime = locomotionState.locomotionAnimPhase01;
			playbackState.playbackTimeSec = locomotionState.locomotionAnimPhase01;
			playbackState.playRate = 1.0f;
			playbackState.loop = locomotionState.mode != LocomotionMode::Idle;
			playbackState.holdLastFrame =
				locomotionState.mode == LocomotionMode::Idle;
		}

		if (RequiresAnimationDirty(previousState, playbackState))
		{
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::Animation);
			}
		}
	}
}

bool ResolveAnimationPlaybackSystem::RequiresAnimationDirty(
	const AnimationPlaybackStateComp& previousState,
	const AnimationPlaybackStateComp& nextState) noexcept
{
	return
		previousState.source != nextState.source ||
		previousState.animationId != nextState.animationId ||
		previousState.boundAbilityInstanceId != nextState.boundAbilityInstanceId ||
		previousState.boundAbilityId != nextState.boundAbilityId ||
		previousState.boundLocomotionMode != nextState.boundLocomotionMode ||
		std::abs(previousState.playRate - nextState.playRate) > kOverlapEpsilon ||
		previousState.loop != nextState.loop ||
		previousState.holdLastFrame != nextState.holdLastFrame;
}
