#pragma once
#include "Component.h"
#include "AnimationSet.h"
#include <cstdint>
#include <memory>
#include <string>
#include <functional>

class Animator;

class AnimationMachine : public Component
{
public:
    void Init() override;
    void Update(float deltaTime) override;

    void SetAnimationSet(shared_ptr<AnimationSet> animSet);

    bool TryPlayClip(const string& clipName);
    void EndCurrentClip();

    void OnServerClipConfirm(
        const string& clipName,
        uint32_t serverAnimId,
        uint32_t abilityInstanceId,
        float serverNormalizedTime);

    string GetCurrentClip() const;
    bool IsPlaying(const string& clipName) const;
    AnimCategory GetCurrentCategory() const;
    shared_ptr<AnimationSet> GetAnimationSet() const;
    uint32_t GetServerAnimId() const;
    uint32_t GetAbilityInstanceId() const;
    float GetCurrentNormalizedTime() const;
    bool HasServerAbilityTiming() const;

    function<string()> onActionEnd;

private:
    bool CanTransition(const ClipInfo* from, const ClipInfo* to) const;
    void PlayClip(const string& clipName);

private:
    Animator* animator = nullptr;
    shared_ptr<AnimationSet> animSet;

    string currentClipName;
    const ClipInfo* currentClip = nullptr;

    uint32_t serverAnimId = 0;
    uint32_t abilityInstanceId = 0;
    float lastServerNormalizedTime = 0.0f;

    bool transitionStarted = false;
};
