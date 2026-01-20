#include "pch.h"
#include "AnimationCommitSystem.h"

void AnimationCommitSystem::Execute(const float dT)
{
	auto& animStates = ecs.GetStorage<AnimationState>();
	auto& animators = ecs.GetStorage<Animator>();
	auto& colliders = ecs.GetStorage<CombatCollider>();

	for (const auto& [entity, animator] : animators)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* animState = animStates.GetComponent(entity);
		auto* collider = colliders.GetComponent(entity);
		if (!animState || !collider) continue;

		const PrebakedAnimation* nextClip =
			AnimationManager::Get().GetAnimation(animState->desiredId);
		if (!nextClip) continue;

		const bool needBind =
			(animator.clip != nextClip) ||
			(collider->staticDatas == nullptr) ||
			(collider->staticDatas != &nextClip->staticDatas);
		
		if (needBind)
		{
			animator.clip = nextClip;
			animator.currentFrame = 0;

			BindColliderToClip(collider, *nextClip);
		}
	}
}

void AnimationCommitSystem::BindColliderToClip(CombatCollider* collider,
	const PrebakedAnimation& clip)
{
	const size_t n = clip.staticDatas.size();

	collider->staticDatas = &clip.staticDatas;

	collider->localDatas.resize(n);
	collider->worldDatas.resize(n);

	collider->enabledMasks.assign(n, 0);
	collider->attackIds.assign(n, 0);
}
