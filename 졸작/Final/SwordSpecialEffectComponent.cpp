#include "pch.h"
#include "SwordSpecialEffectComponent.h"
#include "GameObject.h"
#include "Animator.h"
#include "AnimationMachine.h"
#include "Transform.h"
#include "Engine.h"
#include "EffectManager.h"

void SwordSpecialEffectComponent::Init()
{
	auto owner = GetGameObject();
	animator = owner->GetComponent<Animator>();
	animMachine = owner->GetComponent<AnimationMachine>();
	transform = owner->GetComponent<Transform>();
}

void SwordSpecialEffectComponent::SetBoneIndices(const vector<int>& indices)
{
	if (indices.empty()) return;
	boneIndices = indices;
}

void SwordSpecialEffectComponent::Update(float deltaTime)
{
	if (!animator || !animMachine || !transform) return;
	if (!animator->IsInitialized()) return;
	if (effectName.empty()) return;

	handles.resize(boneIndices.size(), -1);

	const bool playingSpecial = animMachine->IsPlaying("AttackSpecial");
	if (playingSpecial && !wasPlayingSpecial)
	{
		XMFLOAT3 spawnPos = transform->GetPosition();
		for (auto& handle : handles)
			handle = EFFECT_MANAGER->Play(effectName, spawnPos);
	}
	wasPlayingSpecial = playingSpecial;

	XMMATRIX worldMat = transform->GetWorldMatrix();
	XMFLOAT3 playerRot = transform->GetRotation();
	XMVECTOR playerRotQuat = XMQuaternionRotationRollPitchYaw(playerRot.x, playerRot.y, playerRot.z);

	// z축 90도 초기 회전 - DirectX12와 effekseer 축 차이
	XMVECTOR offsetRot = XMQuaternionRotationRollPitchYaw(0, XM_PIDIV2, 0);

	for (size_t i = 0; i < boneIndices.size(); ++i)
	{
		if (handles[i] == -1) continue;

		XMFLOAT3 bonePos = animator->GetBonePosition(boneIndices[i]);
		XMVECTOR boneRot = animator->GetBoneRotation(boneIndices[i]);
		XMVECTOR worldPos = XMVector3TransformCoord(XMLoadFloat3(&bonePos), worldMat);

		// 초기 회전 → 뼈 회전 → 플레이어 회전
		XMVECTOR finalRot = XMQuaternionMultiply(offsetRot, boneRot);
		finalRot = XMQuaternionMultiply(finalRot, playerRotQuat);

		XMMATRIX finalMat = XMMatrixRotationQuaternion(finalRot) * XMMatrixTranslationFromVector(worldPos);
		EFFECT_MANAGER->SetMatrix(handles[i], finalMat);
	}
}
