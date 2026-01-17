#include "pch.h"
#include "AnimationRefSystem.h"

void AnimationRefSystem::Execute(const float dT)
{
	/*auto& animRefs = ecs.GetStorage<AnimationRef>();
	auto& anims = ecs.GetStorage<Animator>();
	auto& colliders = ecs.GetStorage<Collider>();

	for (const auto& [entity, anim] : anims)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* animRef = animRefs.GetComponent(entity);
		auto* collider = colliders.GetComponent(entity);
		if (!animRef || !collider) continue;
		if (!animRef->anim) continue;

		const PrebakedAnimation* clip = animRef->anim;
		int frame = anim.currentFrame;
		if (frame < 0 || frame >= clip->numFrames) continue;

		const auto& localCapsules = clip->frames[frame];
		collider->localCapsules = localCapsules;
	}*/
}