#include "pch.h"
#include "AnimationCommitSystem.h"

void AnimationCommitSystem::Execute(const float dT)
{
	auto& animStates = ecs.GetStorage<AnimationState>();
	auto& animators = ecs.GetStorage<Animator>();
	auto& colliders = ecs.GetStorage<Collider>();

	for (const auto& [entity, animator] : animators)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* animState = animStates.GetComponent(entity);
		auto* collider = colliders.GetComponent(entity);
		if (!animState || !collider) continue;

		const PrebakedAnimation* nextClip =
			AnimationManager::Get().GetAnimation(animState->desiredId);
		if (!nextClip) continue;
		if (animator.clip == nextClip) continue;

		animator.clip = nextClip;
		animator.currentFrame = 0;

		BindColliderToClip(collider, *nextClip);
	}
}

void AnimationCommitSystem::BindColliderToClip(Collider* collider, 
	const PrebakedAnimation& clip)
{
	const size_t n = clip.staticDatas.size();

	collider->staticDatas = &clip.staticDatas;
	collider->staticCount = static_cast<uint32>(n);

	collider->localDatas.resize(n);
	collider->worldDatas.resize(n);

	collider->enabledMasks.assign(n, 0);
	collider->attackIds.assign(n, 0);
}
