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

    // Action/Die 카테고리는 애니메이션 끝나면 Base(Idle)로 복귀
    // TODO: duration 체크 후 자동 전환 로직
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

void AnimationMachine::OnServerClipConfirm(const string& clipName)
{
    // 서버에서 확정된 상태와 현재 예측 상태 비교
    if (currentClipName != clipName)
    {
        // 불일치 시 서버 상태로 보정
        PlayClip(clipName);
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

bool AnimationMachine::CanTransition(const ClipInfo* from, const ClipInfo* to) const
{
    if (!from) return true;

    if (from->category == AnimCategory::Base ||
        from->category == AnimCategory::Special)
        return true;

    return false;
}

void AnimationMachine::PlayClip(const string& clipName)
{
    if (!animSet || !animator) return;

    const ClipInfo* clip = animSet->GetClip(clipName);
    if (!clip || clip->index < 0) return;

    currentClipName = clipName;
    currentClip = clip;

    animator->TransitionToAnimation(clip->index, clip->blendDuration);
}
