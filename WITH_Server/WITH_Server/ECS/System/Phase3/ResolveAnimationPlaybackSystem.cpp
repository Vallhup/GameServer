#include "pch.h"
#include "ResolveAnimationPlaybackSystem.h"

#include "../GameplaySystemUtil.h"
#include "../Phase2/ResolveActionStateSystem.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<5, 0, 1> ResolveAnimationPlaybackSystem::kMetaStorage =
    MakeMetaStorage(
        SysTag<ResolveAnimationPlaybackSystem>(),
        "ResolveAnimationPlaybackSystem",
        std::array<AccessSpec, 5>
        {
            WriteImmediate(ComponentRes<AnimationPlaybackStateComp>()),
            ReadSnapshot(ComponentRes<ActionStateComp>()),
            ReadSnapshot(ComponentRes<LocomotionStateComp>()),
            ReadSnapshot(ComponentRes<SpawnTypeComp>()),
            WriteImmediate(ComponentRes<DirtyFlagsComp>()),
        },
        std::array<SystemTag, 0>{},
        std::array<SystemTag, 1>{ SysTag<ResolveActionStateSystem>() });

void ResolveAnimationPlaybackSystem::Execute(SystemContext& ctx)
{
	for (auto [
		entity,
		playbackState,
		actionState,
		locomotionState,
		spawnType] :
		ctx.ecs.View<
			AnimationPlaybackStateComp,
			ActionStateComp,
			LocomotionStateComp,
			SpawnTypeComp>())
	{
		const AnimationPlaybackStateComp previousState = playbackState;

		if (IsActionActive(actionState))
		{
			const ActionDef* actionDef = FindActionDef(actionState.actionId);
			playbackState.source = AnimationPlaybackSource::Action;
			playbackState.animationId =
				ResolveActionAnimationId(
					spawnType.characterId,
					actionState.actionId);
			playbackState.boundActionInstanceId = actionState.actionInstanceId;
			playbackState.boundActionId = actionState.actionId;
			playbackState.boundLocomotionMode = LocomotionMode::Idle;
			playbackState.playbackTimeSec = actionState.elapsedSec;
			playbackState.normalizedTime =
				(actionDef != nullptr && actionDef->duration > 0.0f)
				? ClampFloat(actionState.elapsedSec / actionDef->duration, 0.0f, 1.0f)
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
			playbackState.boundActionInstanceId = 0;
			playbackState.boundActionId = ActionId::None;
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
		previousState.boundActionInstanceId != nextState.boundActionInstanceId ||
		previousState.boundActionId != nextState.boundActionId ||
		previousState.boundLocomotionMode != nextState.boundLocomotionMode ||
		std::abs(previousState.playRate - nextState.playRate) > kOverlapEpsilon ||
		previousState.loop != nextState.loop ||
		previousState.holdLastFrame != nextState.holdLastFrame;
}

