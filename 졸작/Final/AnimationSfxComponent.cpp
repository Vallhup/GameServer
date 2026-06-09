#include "pch.h"
#include "AnimationSfxComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Engine.h"
#include "SoundManager.h"
#include "EffectManager.h"

void AnimationSfxComponent::AddTrigger(const string& clip, int frameLo, int frameHi, const char* sound)
{
	triggers.push_back({ clip, frameLo, frameHi, sound, false });
}

void AnimationSfxComponent::AddEffectTrigger(const string& clip, int frameLo, int frameHi, const wstring& effect)
{
	effectTriggers.push_back({ clip, frameLo, frameHi, effect, false });
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
			XMFLOAT3 pos{ 0.0f, 0.0f, 0.0f };
			if (auto* transform = owner->GetComponent<Transform>())
				pos = transform->GetPosition();
			SOUND_MANAGER->PlaySFX3D(t.sound, pos);
			t.fired = true;
		}
		else if (!inRange)
		{
			t.fired = false;
		}
	}

	XMFLOAT3 ownerPos{ 0.0f, 0.0f, 0.0f };
	XMMATRIX ownerMat = XMMatrixIdentity();
	if (auto* transform = owner->GetComponent<Transform>())
	{
		ownerPos = transform->GetPosition();
		const XMFLOAT3 rot = transform->GetRotation();
		ownerMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z) * XMMatrixTranslation(ownerPos.x, ownerPos.y, ownerPos.z);
	}

	for (auto& t : effectTriggers)
	{
		const bool playing = animMachine->IsPlaying(t.clip);
		const bool inRange = playing && frame >= t.frameLo && frame <= t.frameHi;

		if (!playing)
			t.suppressed = false;	

		if (inRange && !t.fired && !t.suppressed)
		{
			t.handle = EFFECT_MANAGER->Play(t.effect, ownerPos);
			EFFECT_MANAGER->SetMatrix(t.handle, ownerMat);
			t.fired = true;
		}
		else if (!inRange)
		{
			t.fired = false;
		}

		if (t.handle != -1)
		{
			if (EFFECT_MANAGER->Exists(t.handle))
				EFFECT_MANAGER->SetMatrix(t.handle, ownerMat);
			else
				t.handle = -1;
		}
	}
}

void AnimationSfxComponent::StopEffectByClip(const string& clip)
{
	for (auto& t : effectTriggers)
	{
		if (t.clip != clip) continue;

		t.suppressed = true;	

		if (t.handle != -1)
		{
			if (EFFECT_MANAGER->Exists(t.handle))
				EFFECT_MANAGER->Stop(t.handle);
			t.handle = -1;
		}
	}
}
