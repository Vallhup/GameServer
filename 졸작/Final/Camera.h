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
	void ChangeAngleByInput(float deltaTime);

	XMFLOAT3 GetForward() const;
	XMFLOAT3 GetRight() const;

	void SetCameraPosition(const XMFLOAT3& pos);
	void SetCursor();
	void ChangeCursorInfo(bool in);
	void ReleaseMouse();

private:
	XMFLOAT3 position;
	XMFLOAT3 targetPosition;

	XMFLOAT3 camForward;		// ╬у ╣з
	XMFLOAT3 camRight;				// аб ©Л

	float yaw;
	float pitch;
	float moveSpeed;
	float rotateSpeed;

	int centerX;
	int centerY;

	float mouseSensitivity = 0.1f;
	bool space = false;
};
