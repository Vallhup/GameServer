#include "pch.h"
#include "AnimationSfxComponent.h"
#include "GameObject.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Engine.h"
#include "SoundManager.h"

void AnimationSfxComponent::AddTrigger(const string& clip, int frameLo, int frameHi, const char* sound)
{
	triggers.push_back({ clip, frameLo, frameHi, sound, false });
}

void AnimationSfxComponent::Update(float deltaTime)
{
	auto owner = GetGameObject();
	if (!owner) return;

	auto animMachine = owner->GetComponent<AnimationMachine>();
	auto animator = owner->GetComponent<Animator>();
	if (!animMachine || !animator || !animator->IsInitialized()) return;

	const int frame = animator->GetCurrentFrame();

	for (auto& t : triggers)
	{
		const bool inRange = animMachine->IsPlaying(t.clip) && frame >= t.frameLo && frame <= t.frameHi;

		if (inRange && !t.fired)
		{
			SOUND_MANAGER->PlaySFX(t.sound);
			t.fired = true;
		}
		else if (!inRange)
		{
			t.fired = false;
		}
	}
}
