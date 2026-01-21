#pragma once
#include "Component.h"
#include "AnimationSet.h"
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

    void OnServerClipConfirm(const string& clipName);

    string GetCurrentClip() const;
    bool IsPlaying(const string& clipName) const;
    AnimCategory GetCurrentCategory() const;

    function<string()> onActionEnd;

private:
    bool CanTransition(const ClipInfo* from, const ClipInfo* to) const;
    void PlayClip(const string& clipName);

private:
    Animator* animator = nullptr;
    shared_ptr<AnimationSet> animSet;

    string currentClipName;
    const ClipInfo* currentClip = nullptr;

    bool transitionStarted = false;
};
