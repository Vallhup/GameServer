#pragma once

#include "ECS.h"
#include "System.h"

class AnimationTimeSystem : public System {
public:
	AnimationTimeSystem(ECS& e, int p = 0) : System(e, p) {}
	virtual ~AnimationTimeSystem() = default;

	virtual void Execute(const float dT) override
	{
		auto& states = ecs.GetStorage<AnimationState>();
		auto& animRefs = ecs.GetStorage<AnimationRef>();
		auto& anims = ecs.GetStorage<Animator>();

		for (const auto& [entity, state] : states)
		{
			auto* animRef = animRefs.GetComponent(entity);
			auto* anim = anims.GetComponent(entity);
			if (!animRef or !anim) continue;
			if (!animRef->anim) continue;

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

	virtual std::vector<std::type_index> ReadComponents() const 
	{ 
		return { typeid(AnimationRef) };
	}

	virtual std::vector<std::type_index> WriteComponents() const 
	{ 
		return { typeid(AnimationState), typeid(Animator) };
	}
};

