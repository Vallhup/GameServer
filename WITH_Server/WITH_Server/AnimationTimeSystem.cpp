#include "pch.h"
#include "AnimationTimeSystem.h"

void AnimationTimeSystem::Execute(const float dT)
{
	auto& actions = ecs.GetStorage<ActionState>();
	auto& animRefs = ecs.GetStorage<AnimationRef>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& anims = ecs.GetStorage<Animator>();

    for (const auto& [e, animator] : anims)
    {
        if (ecs.GetStorage<DisconnectedTag>().HasComponent(e)) continue;

        auto* action = actions.GetComponent(e);
        auto* loco = locos.GetComponent(e);
        auto* ar = animRefs.GetComponent(e);
        if (!action || !loco || !ar || !ar->anim) continue;

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
        // 2) 액션이 없을 때(Idle/Walk): locomotion phase 필요(거리/속도 기반)
        else
        {
            // 예시: 이동 중이면 전역시간/속도에서 위상을 만들거나,
            // 이동거리 누적으로 위상을 만들 것(여기서는 스케치만).
            // frame = ...
        }

        if (frame < 0) frame = 0;
        if (frame >= clip->numFrames) frame = clip->numFrames - 1;
        animator.currentFrame = frame;
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