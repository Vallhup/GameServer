#include "AnimationTimeSystem.h"

void AnimationTimeSystem::Execute(const float dT)
{
	auto& states = ecs.GetStorage<AnimationState>();
	auto& animRefs = ecs.GetStorage<AnimationRef>();
	auto& anims = ecs.GetStorage<Animator>();

	for (const auto& [entity, state] : states)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* animRef = animRefs.GetComponent(entity);
		auto* anim = anims.GetComponent(entity);
		if (!animRef or !anim) continue;
		if (!animRef->anim) continue;

		// TEMP : 추후 time 계산 방식 변경 필요
		//        ActionState 기반으로 변경
		const PrebakedAnimation* clip = animRef->anim;
		state.time += dT * state.speed;

		const float duration = clip->numFrames / clip->fps;

		if (state.looping)
		{
			if (state.time >= duration)
				state.time = fmodf(state.time, duration);
		}

		else
		{
			if (state.time >= duration)
				state.time = duration - 1e-6f;
		}

		int frame = static_cast<int>(state.time * clip->fps);
		if (frame < 0) frame = 0;
		if (frame >= clip->numFrames) frame = clip->numFrames - 1;

		anim->currentFrame = frame;
	}
}

std::vector<std::type_index> AnimationTimeSystem::ReadComponents() const
{
	return { typeid(AnimationRef), typeid(ActionState) };
}

std::vector<std::type_index> AnimationTimeSystem::WriteComponents() const
{
	return { typeid(AnimationState), typeid(Animator) };
}