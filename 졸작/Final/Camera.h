#pragma once

class DX12Core;

class Camera
{
public:
	void Initialize();
	void Update(DX12Core& core, float deltaTime);
	void UpdateInputtoCamLogic(float deltaTime);
	void UpdateCameraMatrices(DX12Core& core);

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
