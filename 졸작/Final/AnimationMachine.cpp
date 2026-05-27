#include "pch.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "GameObject.h"

void AnimationMachine::Init()
{
    animator = GetGameObject()->GetComponent<Animator>();
}

void AnimationMachine::Update(float deltaTime)
{
    if (!currentClip) return;

    if (currentClip->category == AnimCategory::Action)
    {
        float progress = animator->GetAnimationProgress();

        if (progress >= 0.95f && !transitionStarted)
        {
            transitionStarted = true;

            EndCurrentClip();
        }
    }
}

void AnimationMachine::SetAnimationSet(shared_ptr<AnimationSet> set)
{
    animSet = set;
}

bool AnimationMachine::TryPlayClip(const string& clipName)
{
    if (!animSet) return false;

    const ClipInfo* targetClip = animSet->GetClip(clipName);
    if (!targetClip) return false;

    if (!CanTransition(currentClip, targetClip)) return false;

    PlayClip(clipName);
    return true;
}

void AnimationMachine::EndCurrentClip()
{
    string nextClip = onActionEnd ? onActionEnd() : "Idle";
    PlayClip(nextClip);
}

void AnimationMachine::OnServerClipConfirm(
    const string& clipName,
    uint32_t newServerAnimId,
    uint32_t newAbilityInstanceId,
    float newServerNormalizedTime)
{
    serverAnimId = newServerAnimId;
    abilityInstanceId = newAbilityInstanceId;
    lastServerNormalizedTime = newServerNormalizedTime;

    if (currentClipName != clipName)
    {
        PlayClip(clipName);

        if (animator)
        {
            animator->SetAnimationProgress(lastServerNormalizedTime);
        }
    }
}

string AnimationMachine::GetCurrentClip() const
{
    return currentClipName;
}

bool AnimationMachine::IsPlaying(const string& clipName) const
{
    return currentClipName == clipName;
}

AnimCategory AnimationMachine::GetCurrentCategory() const
{
    if (currentClip)
        return currentClip->category;
    return AnimCategory::Base;
}

shared_ptr<AnimationSet> AnimationMachine::GetAnimationSet() const
{
    return animSet;
}

uint32_t AnimationMachine::GetServerAnimId() const
{
    return serverAnimId;
}

uint32_t AnimationMachine::GetAbilityInstanceId() const
{
    return abilityInstanceId;
}

float AnimationMachine::GetCurrentNormalizedTime() const
{
    if (!animator)
    {
        return 0.0f;
    }

    return std::clamp(animator->GetAnimationProgress(), 0.0f, 1.0f);
}

bool AnimationMachine::HasServerAbilityTiming() const
{
    return
        currentClip != nullptr &&
        currentClip->category == AnimCategory::Action &&
        serverAnimId != 0 &&
        abilityInstanceId != 0;
}

bool AnimationMachine::CanTransition(const ClipInfo* from, const ClipInfo* to) const
{
    if (!from) return true;

    if (from->category == AnimCategory::Base)
        return true;

    if (from->category == AnimCategory::Special)
        return (to->category == AnimCategory::Action);

    return false;
}

void AnimationMachine::PlayClip(const string& clipName)
{
    if (!animSet || !animator) return;

    const ClipInfo* clip = animSet->GetClip(clipName);
    if (!clip || clip->index < 0) return;

    currentClipName = clipName;
    currentClip = clip;

    transitionStarted = false;

    animator->TransitionToAnimation(clip->index, clip->blendDuration);
    animator->SetLoop(clip->category != AnimCategory::Die);
}
