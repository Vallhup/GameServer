#pragma once

class DX12Core;
class GameObject;
class MainCharacter;
class InstancingBatch;
class Terrain;

class Camera
{
public:
	~Camera();

	void Initialize(HWND hWnd);
	void InitCameraPositionFromCharacter(const XMFLOAT3& pos, float charYawRad);

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

	UINT GetLutIndex() const { return lutIndex; }
	float GetSaturation() const { return toneSaturationFactor; }
	void SetLutPreset(UINT idx, float saturation);

	void SetMouseSensitivity(float s) { mouseSensitivity = s; }
	void SetBrightness(float b) { screenBrightness = b; }
	void SetSaturation(float s) { toneSaturationFactor = s; }

	void SetCameraPosition(const XMFLOAT3& pos);

	void SetTerrain(const Terrain* t) { terrain = t; }

	void SetCinematicView(DX12Core& core, const XMFLOAT3& eye, const XMFLOAT3& lookAt);

	void EnterFocusView(const XMFLOAT3& eye, const XMFLOAT3& lookAt);
	void ExitFocusView();
	bool IsFocusView() const { return focusActive; }

	void SetCursor(bool in);
	void ReleaseMouse();

	void AddTrauma(float amount);

	void TriggerDodgeZoom();        

	bool IsCursorActive() const { return spacePressed; }

private:
	void UpdateInputtoCamLogic(DX12Core& core, float deltaTime);
	void UpdateSmoothFollow(float deltaTime);

	void UpdateShake(float deltaTime);
	XMFLOAT3 GetShakeOffset() const;

	void UpdateZoomKick(float deltaTime);
	float GetZoomKick() const;
	float GetZoomBell() const;

	void UpdateFocusView(DX12Core& core, float deltaTime);

	void UpdateCameraMatrices(DX12Core& core);
	void UpdateForwardAndRight();
	void ChangeAngleByInput(float deltaTime);

	void UpdatePosByObstruction(const vector<shared_ptr<GameObject>>& sceneObjects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const shared_ptr<MainCharacter>& myPlayer);
	bool CheckObstruction(const vector<shared_ptr<GameObject>>& objects, const vector<shared_ptr<InstancingBatch>>& instancingBatches, const XMFLOAT3& targetPos, float& adjustedDistance, const shared_ptr<MainCharacter>& myPlayer);
	
	void TestObjectObstruction(const shared_ptr<GameObject>& obj, FXMVECTOR rayOrigin, FXMVECTOR rayDir, float maxDistance, float& closestDistance, bool& foundObstruction);
	static bool RayTriangleNearest(const vector<XMFLOAT3>& positions, const vector<UINT>& indices, FXMVECTOR origin, FXMVECTOR dir, float maxDistance, float& outDist);

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

	float mouseSensitivity = 0.1f;       
	static constexpr float CAMERA_FOLLOW_SPEED = 120.0f;
	static constexpr float TARGET_FOLLOW_SPEED = 6.0f;
	static constexpr float TERRAIN_CLEARANCE = 0.001f;   

	float desiredDistance;
	float currentDistance;
	float zoomDistance;       
	float minZoomDistance = 3.0f;       
	float minCollisionDistance = 1.5f;  
	float maxDistance = 4.5f;
	float zoomSpeedPerNotch = 0.25f;    
	float zoomFollowSpeed = 2.5f;

	float shakeTrauma = 0.0f;
	float shakeTime = 0.0f;
	static constexpr float SHAKE_DECAY = 2.0f;        
	static constexpr float SHAKE_MAX_OFFSET = 0.18f;  
	static constexpr float SHAKE_FREQUENCY = 28.0f;

	float zoomKickTime = -1.0f;                         
	float zoomLevel = 0.0f;                             
	static constexpr float ZOOM_KICK_AMPLITUDE = -0.7f; 
	static constexpr float ZOOM_KICK_START = 0.25f;     
	static constexpr float ZOOM_KICK_HOLD = 0.72f;      
	static constexpr float ZOOM_ATTACK = 6.0f;          
	static constexpr float ZOOM_RELEASE = 1.3f;         
	static constexpr float ZOOM_KICK_RECENTER = 1.0f;   

	BoundingFrustum viewFrustum;

	XMFLOAT4X4 matView;
	XMFLOAT4X4 matProj;

	UINT lutIndex = 0;
	UINT prevLutIndex = 0;
	float lutBlendFactor = 1.0f;
	float lutTransitionSpeed = 2.0f;
	float toneSaturationFactor = 0.85f;
	float screenBrightness = 1.0f;       

	const Terrain* terrain = nullptr;

	bool focusActive = false;
	bool focusExiting = false;
	float focusT = 0.0f;
	XMFLOAT3 focusEye{};
	XMFLOAT3 focusLookAt{};
	static constexpr float FOCUS_DURATION = 0.6f;
};
