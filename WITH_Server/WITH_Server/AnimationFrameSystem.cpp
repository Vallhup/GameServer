#include "pch.h"
#include "AnimationFrameSystem.h"

void AnimationFrameSystem::Execute(const float dT)
{
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& animStates = ecs.GetStorage<AnimationState>();
	auto& animators = ecs.GetStorage<Animator>();
	auto& locoPhases = ecs.GetStorage<LocomotionAnimPhase>();

	for (const auto& [entity, animator] : animators)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* actionState = actionStates.GetComponent(entity);
		auto* animState = animStates.GetComponent(entity);
		auto* locoPhase = locoPhases.GetComponent(entity);
		if (!actionState || !animState || !locoPhase) continue;

		const PrebakedAnimation* clip = animator.clip;
		if (!clip || clip->numFrames == 0) continue;

		uint16 frame{ 0 };
		if (actionState->type != ActionType::None)
			frame = static_cast<uint16>(actionState->elapsed * clip->fps);

		else
			frame = static_cast<uint16>(locoPhase->phase * clip->numFrames);

		if (animState->looping)
			frame = frame % clip->numFrames;

		else if (frame >= clip->numFrames)
			frame = clip->numFrames - 1;

		animator.currentFrame = frame;
	}
}