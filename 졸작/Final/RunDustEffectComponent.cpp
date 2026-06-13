#include "pch.h"
#include "RunDustEffectComponent.h"
#include "GameObject.h"
#include "Animator.h"
#include "AnimationMachine.h"
#include "Transform.h"
#include "Engine.h"
#include "EffectManager.h"

void RunDustEffectComponent::Init()
{
	auto owner = GetGameObject();
	animator = owner->GetComponent<Animator>();
	animMachine = owner->GetComponent<AnimationMachine>();
	transform = owner->GetComponent<Transform>();
}

void RunDustEffectComponent::AddFootstep(int frameLo, int frameHi, int boneIndex)
{
	footsteps.push_back({ frameLo, frameHi, boneIndex, false });
}

void RunDustEffectComponent::Update(float deltaTime)
{
	if (!animator || !animMachine || !transform) return;
	if (!animator->IsInitialized()) return;

	const bool running = animMachine->IsPlaying("Run");
	const int frame = animator->GetCurrentFrame();
	const XMMATRIX worldMat = transform->GetWorldMatrix();

	for (auto& fs : footsteps)
	{
		const bool inRange = running && frame >= fs.frameLo && frame <= fs.frameHi;

		if (inRange && !fs.fired)
		{
			XMFLOAT3 bonePos = animator->GetBonePosition(fs.boneIndex);
			XMFLOAT3 spawnPos;
			XMStoreFloat3(&spawnPos, XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat));
			EFFECT_MANAGER->Play(L"RunDust", spawnPos);
			fs.fired = true;
		}
		else if (!inRange)
		{
			fs.fired = false;
		}
	}
}
