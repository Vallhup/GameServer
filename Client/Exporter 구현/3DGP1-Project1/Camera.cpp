#include "pch.h"
#include "Camera.h"
#include "GameObject.h"

void Camera::Initialize(const GameObject* obj, const XMFLOAT3& sceneoffset, const XMVECTOR& value)
{
	mTarget = obj;

	mTransform.SetRotationVec(value);
	mTransform.SetPositionVec(mTarget->GetTransform().GetPositionVec() + XMLoadFloat3(&sceneoffset));

	originalSceneOffset = sceneoffset;
}

void Camera::Update(const float deltaTime, const XMFLOAT3& sceneoffset, const float speed)
{
	UpdateShake(deltaTime);

	XMVECTOR pos = mTransform.GetPositionVec();
	XMVECTOR baseOffset = XMLoadFloat3(&sceneoffset);

	if (isShaking)
	{
		baseOffset = XMVectorAdd(baseOffset, shakeOffset);
	}

	XMVECTOR toPos = baseOffset + mTarget->GetTransform().GetPositionVec();
	mTransform.SetPositionVec(XMVectorLerp(pos, toPos, speed * deltaTime));
}

Transform& Camera::GetTransform()
{
	return mTransform;
}

const Transform& Camera::GetTransform() const
{
	return mTransform;
}

void Camera::SetTransform(const Transform& transform)
{
	mTransform = transform;
}

void Camera::StartShake(float intensity, float duration)
{
	isShaking = true;
	shakeIntensity = intensity;
	shakeDuration = duration;
	shakeTimer = 0.0f;
	shakeOffset = XMVectorZero();
}

void Camera::UpdateShake(float deltaTime)
{
	if (!isShaking) return;

	shakeTimer += deltaTime;

	if (shakeTimer >= shakeDuration)
	{
		isShaking = false;
		shakeOffset = XMVectorZero();
		return;
	}

	float currentIntensity = shakeIntensity * (1.0f - shakeTimer / shakeDuration);

	float randomX = ((rand() % 2000) - 1000) / 1000.0f; // -1.0 ~ 1.0
	float randomY = ((rand() % 2000) - 1000) / 1000.0f; // -1.0 ~ 1.0
	float randomZ = ((rand() % 2000) - 1000) / 1000.0f; // -1.0 ~ 1.0

	shakeOffset = XMVectorSet(
		randomX * currentIntensity,
		randomY * currentIntensity,
		randomZ * currentIntensity,
		0.0f
	);
}

