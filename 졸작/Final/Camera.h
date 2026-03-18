#pragma once

class DX12Core;
class GameObject;
class MainCharacter;
class InstancingBatch;

class Camera
{
public:
	void Initialize(HWND hWnd);
	void InitCameraPositionFromCharacter(const XMFLOAT3& pos);

	void Update(DX12Core& core, float deltaTime, const vector<shared_ptr<GameObject>>& sceneObjects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const shared_ptr<MainCharacter>& myPlayer);

	XMFLOAT3 GetForward() const;
	XMFLOAT3 GetRight() const;
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetTargetPosition() const;
	float GetRadianYaw() const;
	float GetRadianPitch() const;

	BoundingFrustum GetViewFrustum() const;
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

	void SetCameraPosition(const XMFLOAT3& pos);
	void SetCursor();
	void ReleaseMouse();

private:
	void UpdateInputtoCamLogic(float deltaTime);
	void UpdateSmoothFollow(float deltaTime);
	void UpdateCameraMatrices(DX12Core& core);
	void UpdateForwardAndRight();
	void ChangeAngleByInput(float deltaTime);

	void UpdatePosByObstruction(const vector<shared_ptr<GameObject>>& sceneObjects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const shared_ptr<MainCharacter>& myPlayer);
	bool CheckObstruction(const vector<shared_ptr<GameObject>>& objects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const XMFLOAT3& targetPos, float& adjustedDistance, const shared_ptr<MainCharacter>& myPlayer);

	void ChangeCursorInfo(bool in);

private:
	HWND hwnd;

	XMFLOAT3 position;
	XMFLOAT3 targetPosition;

	XMFLOAT3 desiredPosition;      
	XMFLOAT3 currentTargetPos;     
	XMFLOAT3 desiredTargetPos;     

	XMFLOAT3 camForward;			
	XMFLOAT3 camRight;			

	float yaw;
	float pitch;
	float moveSpeed;
	float rotateSpeed;

	bool spacePressed = false;

	static constexpr float MOUSE_SENSITIVITY = 0.1f;
	static constexpr float CAMERA_FOLLOW_SPEED = 40.0f;		
	static constexpr float TARGET_FOLLOW_SPEED = 4.0f;		

	float desiredDistance;   
	float currentDistance;   
	float minDistance = 0.5f;
	float maxDistance = 4.5f;
	float zoomSpeedPerNotch = 0.25f;    
	float zoomFollowSpeed = 2.5f;

	BoundingFrustum viewFrustum;

	XMFLOAT4X4 matView;
	XMFLOAT4X4 matProj;

	UINT lutIndex = 0;
};
