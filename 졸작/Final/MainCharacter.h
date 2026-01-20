#pragma once
#include "GameObject.h"

class Camera;

class MainCharacter : public GameObject
{
public:
	void Update(float deltaTime) override;

	void SetCamera(Camera* cam);

private:
	void BasicMove();
	void BasicAttack();
	void BasicDodge();

private:
	Camera* camera = nullptr;

	XMFLOAT3 characterForward;
	XMFLOAT3 characterRight;

	float currentYawAngle = 0.0f;
	float targetYawAngle = 0.0f;
	bool needsRotation = false;

	static constexpr float MOVE_SPEED = 2.0f;
	static constexpr float ROT_SPEED = 3.14f;
};

