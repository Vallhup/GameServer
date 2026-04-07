#include "pch.h"
#include "ResolveAnimationPlaybackSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveAnimationPlaybackSystem::kMeta =
	MakeSystemMeta<ResolveAnimationPlaybackSystem>(
		"ResolveAnimationPlaybackSystem");

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
		if (IsActionActive(actionState))
		{
			const ActionDef* actionDef = FindActionDef(actionState.actionId);
			playbackState.source = AnimationPlaybackSource::Action;
			playbackState.animationId =
				ResolveActionAnimationId(actionState.actionId);
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

		if (DirtyFlagsComp* dirty =
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
		{
			dirty->MarkDirty(WorldDirtyType::Animation);
		}
	}
}

const SystemMeta& ResolveAnimationPlaybackSystem::Meta() const
{
	return kMeta;
}
