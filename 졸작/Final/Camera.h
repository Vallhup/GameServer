#pragma once

class Camera
{
public:
	static Camera& Get();

	void Initialize();
	void Update(float deltaTime);
	void UpdateInputtoCamLogic(float deltaTime);
	void ApplyToCB();

	void UpdateForwardAndRight();
	void ChangePosByInput(float deltaTime);
	void ChangeAngleByInput(float deltaTime);

private:
	XMFLOAT3 position;
	XMFLOAT3 camForward;		// ╬у ╣з
	XMFLOAT3 right;			// аб ©Л

	float yaw;
	float pitch;
	float moveSpeed;
	float rotateSpeed;
};
