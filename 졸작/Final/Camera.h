#pragma once

class DX12Core;

class Camera
{
public:
	void Initialize();
	void InitCameraPositionFromCharacter(const XMFLOAT3& pos);

	void Update(DX12Core& core, float deltaTime);
	void UpdateInputtoCamLogic(float deltaTime);
	void UpdateCameraMatrices(DX12Core& core);

	void UpdateForwardAndRight();
	void ChangePosByInput(float deltaTime);
	void ChangeAngleByInput(float deltaTime);

	void SetCameraPosition(const XMFLOAT3& pos);

private:
	XMFLOAT3 position;
	XMFLOAT3 targetPosition;

	XMFLOAT3 camForward;		// ╬у ╣з
	XMFLOAT3 right;			// аб ©Л

	float yaw;
	float pitch;
	float moveSpeed;
	float rotateSpeed;

	XMFLOAT2 lastMousePos;
	float mouseSensitivity = 0.25f;
};
