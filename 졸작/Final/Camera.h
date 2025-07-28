#pragma once
#include "Transform.h"

class GameObject;

class Camera
{
public:
	Camera() = default;
	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;
	~Camera() = default;

	void Initialize(const GameObject* obj, const XMFLOAT3& sceneoffset, const XMVECTOR& value);
	void Update(const float deltaTime, const XMFLOAT3& sceneoffset, const float speed);

	Transform& GetTransform();
	const Transform& GetTransform() const;
	void SetTransform(const Transform& transform);

	void StartShake(float intensity, float duration);
	void UpdateShake(float deltaTime);

private:
	Transform mTransform = {};
	const GameObject* mTarget = nullptr;

	bool isShaking = false;
	float shakeIntensity = 0.0f;
	float shakeDuration = 0.0f;
	float shakeTimer = 0.0f;
	XMFLOAT3 originalSceneOffset = {};
	XMVECTOR shakeOffset = {};
};
