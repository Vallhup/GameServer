#include "pch.h"
#include "PotionAttachComponent.h"
#include "GameObject.h"
#include "Animator.h"
#include "AnimationMachine.h"
#include "Transform.h"

void PotionAttachComponent::Init()
{
	auto owner = GetGameObject();
	animator = owner->GetComponent<Animator>();
	animMachine = owner->GetComponent<AnimationMachine>();
	transform = owner->GetComponent<Transform>();

	BuildLocalAdjust(6.7f, 2.0f, 2.8f);   
}

void PotionAttachComponent::BuildLocalAdjust(float x, float y, float z)
{
	XMMATRIX adjust = XMMatrixRotationRollPitchYaw(0.252636f, -0.028763f, 3.151817f) *
		XMMatrixTranslation(x, y, z);
	XMStoreFloat4x4(&localAdjust, adjust);
}

void PotionAttachComponent::Update(float deltaTime)
{
	auto p = potion.lock();
	if (!p) return;

	auto potionTransform = p->GetComponent<Transform>();
	if (!potionTransform) return;

	const bool drinking = animator && animMachine && transform
		&& animator->IsInitialized() && boneIndex >= 0
		&& animMachine->IsPlaying("Drink");

	if (!drinking)
	{
		p->SetId(-1);   
		return;
	}

	XMMATRIX boneWorld = animator->GetBoneWorldMatrix(boneIndex);
	XMMATRIX charWorld = transform->GetWorldMatrix();
	XMMATRIX world = XMLoadFloat4x4(&localAdjust) * boneWorld * charWorld;

	potionTransform->SetWorldOverride(world);
	p->SetId(kVisibleId);
}
