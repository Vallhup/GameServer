#include "pch.h"
#include "AnimationPoseBindSystem.h"
#include "Tags.h"

void AnimationPoseBindSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	const auto& animators = ecs.GetStorage<Animator>();
	auto& colliders = ecs.GetStorage<CombatCollider>();

	for (const auto& [entity, animator] : animators)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* collider = colliders.GetComponent(entity);
		if (!collider) continue;

		const PrebakedAnimation* clip = animator.clip;
		if (!clip) continue;

		const uint16 frame = animator.currentFrame;
		if (frame >= clip->numFrames) continue;

		const auto& dynamicData = clip->dynamicDatas[frame];

#ifdef _DEBUG
		const uint64 n = clip->staticDatas.size();
		assert(collider->staticDatas == &clip->staticDatas);
		assert(collider->localDatas.size() == n);
		assert(dynamicData.size() == n);
#endif
		
		std::copy(dynamicData.begin(), dynamicData.end(), collider->localDatas.begin());
	}
}