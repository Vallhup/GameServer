#include "pch.h"
#include "AnimationTimeSystem.h"

void AnimationTimeSystem::Execute(const float dT)
{
	auto& actions = ecs.GetStorage<ActionState>();
	auto& animRefs = ecs.GetStorage<AnimationRef>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& anims = ecs.GetStorage<Animator>();
    auto& animPhases = ecs.GetStorage<LocomotionAnimPhase>();

    for (const auto& [e, animator] : anims)
    {
        if (ecs.GetStorage<DisconnectedTag>().HasComponent(e)) continue;

        auto* action = actions.GetComponent(e);
        auto* loco = locos.GetComponent(e);
        auto* ar = animRefs.GetComponent(e);
		auto* animPhase = animPhases.GetComponent(e);
        if (!action || !loco || !ar 
            || !ar->anim || !animPhase) continue;

        const PrebakedAnimation* clip = ar->anim;
        const float clipDur = clip->numFrames / clip->fps;

        int frame = 0;
        if (action->type != ActionType::None)
        {
            if (std::isinf(action->duration))
            {
                float t = fmodf(action->elapsed, clipDur);
                frame = (int)(t * clip->fps);
            }

            else
            {
                float p = (action->duration > 0.0f) ? (action->elapsed / action->duration) : 0.0f;
                p = std::clamp<float>(p, 0.0f, 1.0f);
                frame = static_cast<int>(p * (clip->numFrames - 1));
            }
        }

        else
        {
            if (!loco->isMoving) frame = 0;
            else
            {
                frame = static_cast<int>(animPhase->phase * clip->numFrames);
                frame = frame % clip->numFrames;
                if (frame < 0) frame += clip->numFrames;
            }
        }

		frame = std::clamp<int>(frame, 0, clip->numFrames - 1);
        animator.currentFrame = frame;
    }
}