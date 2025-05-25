#pragma once
#include "Scene.h"

struct ControlPoint {
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	bool isCurve;
};

class RollerCoster final : public Scene
{
public:
	RollerCoster() = default;
	RollerCoster(const RollerCoster&) = delete;
	RollerCoster& operator=(const RollerCoster&) = delete;
	~RollerCoster() = default;

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
	void InitializeProjection() override;
	void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
	void UpdateLogic(const float deltaTime) override;
	const GameObject* GetWorld() const override;
	int GetSceneWidth() const override;

private:
	void InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void InitializeRCBody();
	void InitializeCart();
	void InitializeCamera();

	void HandleInput();
	void UpdateTrackProgress(const float deltaTime);
	void UpdateCartPosition();
	void NormalizeRotation(XMFLOAT3& rotation);

	XMFLOAT3 InterpolatePosition(const XMFLOAT3& pos1, const XMFLOAT3& pos2);
	XMFLOAT3 InterpolateRotation(const XMFLOAT3& rot1, const XMFLOAT3& rot2);

	XMFLOAT3 CalculateCartPosition(const XMFLOAT3& trackPos, const XMFLOAT3& trackRot);
	XMFLOAT3 CalculateCartRotation(const XMFLOAT3& trackRot);
	XMFLOAT3 CalculateDirection(const XMFLOAT3& start, const XMFLOAT3& end);
	float CalculateDistance(const XMFLOAT3& direction);


	vector<ControlPoint> DefineControlPoints();

	void CreateTrackFromControlPoints(const std::vector<ControlPoint>& controlPoints);
	void CreateTrackPiece(const XMFLOAT3& position, const XMFLOAT3& rotation);
	void CreateStraightTrack(const ControlPoint& current, const ControlPoint& next, int numPieces);
	void CreateCurvedTrack(const ControlPoint& current, const ControlPoint& next, float distance, int arcSegments);
	void CreateYAxisCurvedTrack(const ControlPoint& current, const ControlPoint& next, float distance, float angleYDiff, float angleZDiff, int arcSegments);
	void CreateZAxisCurvedTrack(const ControlPoint& current, const ControlPoint& next, float distance, float angleZDiff, int arcSegments);

private:
	unique_ptr<VertexIndexBuffer> cube = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> body = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> human = make_unique<VertexIndexBuffer>();
	unique_ptr<VertexIndexBuffer> hair = make_unique<VertexIndexBuffer>();
	VertexIndexBuffer rCube = {};
	VertexIndexBuffer rCartBody = {};

	GameObject rWorld = {};
	GameObject rCenter = {};
	GameObject rCart = {};
	GameObject rHumanBody = {};
	GameObject rHumanHead = {};
	GameObject rHumanNose = {};
	GameObject rHumanHair = {};
	GameObject rBody = {};

	int mCurrentTrackIndex = 0;     
	float mTrackSpeed = 30.0f;       
	float mTrackProgress = 0.0f;	

	vector<GameObject*> rTrackPieces = {};

	const float color[4] = { 0.8156f, 0.8901f, 0.9294f, 1.0f };

	static constexpr XMFLOAT3 RCSCENE_OFFSET = { 0.0f, 0.4f, -0.4f };
	static constexpr int ROLLERCOSTER_GAME_WIDTH = 2;
};

