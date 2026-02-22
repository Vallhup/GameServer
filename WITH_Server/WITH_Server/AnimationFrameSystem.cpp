#include "pch.h"
#include "AnimationFrameSystem.h"
#include "Tags.h"

void AnimationFrameSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	const auto& actionStates = ecs.GetStorage<ActionState>();
	const auto& animStates = ecs.GetStorage<AnimationState>();
	const auto& locoPhases = ecs.GetStorage<LocomotionAnimPhase>();
	auto& animators = ecs.GetStorage<Animator>();

	for (const auto& [entity, animator] : animators)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* actionState = actionStates.GetComponent(entity);
		const auto* animState = animStates.GetComponent(entity);
		const auto* locoPhase = locoPhases.GetComponent(entity);
		if (!actionState || !animState || !locoPhase) continue;

		const PrebakedAnimation* clip = animator.clip;
		if (!clip || clip->numFrames == 0) continue;

		uint16 frame{ 0 };
		if (actionState->action != ActionType::None)
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