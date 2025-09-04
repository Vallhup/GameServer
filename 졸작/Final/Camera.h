#pragma once

class DX12Core;

class Camera
{
public:
	void Initialize();
	void InitCameraPositionFromCharacter(const XMFLOAT3& pos);

	void Update(DX12Core& core, float deltaTime);
	void UpdateInputtoCamLogic(float deltaTime);
	void UpdateSmoothFollow(float deltaTime);
	void UpdateCameraMatrices(DX12Core& core);

	void UpdateForwardAndRight();
	void ChangeAngleByInput(float deltaTime);

	XMFLOAT3 GetForward() const;
	XMFLOAT3 GetRight() const;
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetTargetPosition() const;

	void SetCameraPosition(const XMFLOAT3& pos);
	void SetCursor();
	void ChangeCursorInfo(bool in);
	void ReleaseMouse();

private:
	XMFLOAT3 position;
	XMFLOAT3 targetPosition;

	XMFLOAT3 desiredPosition;      // 목표하는 카메라 위치
	XMFLOAT3 currentTargetPos;     // 현재 추적 중인 타겟 위치
	XMFLOAT3 desiredTargetPos;     // 목표하는 타겟 위치

	XMFLOAT3 camForward;			// 앞 뒤
	XMFLOAT3 camRight;				// 좌 우

	float yaw;
	float pitch;
	float moveSpeed;
	float rotateSpeed;

	int centerX;
	int centerY;

	bool spacePressed = false;

	static constexpr float MOUSE_SENSITIVITY = 0.1f;
	static constexpr float CAMERA_FOLLOW_SPEED = 40.0f;		// 카메라 위치 보간 속도
	static constexpr float TARGET_FOLLOW_SPEED = 4.0f;		// 캐릭터 위치 보간 속도
};
