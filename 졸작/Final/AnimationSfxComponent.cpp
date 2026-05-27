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

	for (auto& t : effectTriggers)
	{
		const bool inRange = animMachine->IsPlaying(t.clip) && frame >= t.frameLo && frame <= t.frameHi;

		if (inRange && !t.fired)
		{
			XMFLOAT3 pos{ 0.0f, 0.0f, 0.0f };
			XMFLOAT3 rot{ 0.0f, 0.0f, 0.0f };
			if (auto* transform = owner->GetComponent<Transform>())
			{
				pos = transform->GetPosition();
				rot = transform->GetRotation();
			}

			Effekseer::Handle h = EFFECT_MANAGER->Play(t.effect, pos);
			const XMMATRIX mat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z) * XMMatrixTranslation(pos.x, pos.y, pos.z);

			EFFECT_MANAGER->SetMatrix(h, mat);
			t.fired = true;
		}
		else if (!inRange)
		{
			t.fired = false;
		}
	}
}
