#pragma once
#include "GameObject.h"

class Camera;

class MainCharacter : public GameObject
{
public:
	void Update(float deltaTime) override;
	void UpdateMovementDirections();
	void BasicMove(float deltaTime);

	void SetCamera(Camera* cam);

private:
	Camera* camera = nullptr;

	XMFLOAT3 characterForward;
	XMFLOAT3 characterRight;

	float currentYawAngle = 0.0f;
	float targetYawAngle = 0.0f;
	bool needsRotation = false;

	int currentAnimState = 1;

	static constexpr float MOVE_SPEED = 2.0f;
	static constexpr float ROT_SPEED = 3.14f;
};

