#pragma once

class Mesh;
class GameObject;
class DX12Core;
class SceneRenderer;

struct CachedInstanceData
{
	XMMATRIX worldMatrix;
	XMFLOAT3 position;
	BoundingOrientedBox boundingBox;
	float cullDistance;
	bool needDistanceCull;
};

class InstancingBatch
{
public:
	void Initialize(Mesh* m);
	void AddObject(shared_ptr<GameObject> obj);
	void BuildBuffers(DX12Core& core);
	void Update(const BoundingFrustum& frustum, const XMVECTOR& camPos, const XMVECTOR& playerPos);
	void Render(DX12Core& core, SceneRenderer* renderer);
	void RenderShadowStatic(DX12Core& core, SceneRenderer* renderer);
	void Clear();

	const vector<shared_ptr<GameObject>>& GetObjects() const { return objects; }

	D3D12_GPU_VIRTUAL_ADDRESS GetCBAddress(size_t idx) const { return objectCBs[idx]->GetGPUVirtualAddress(); }
	void SetCastShadow(bool in) { castShadow = in; }
	void SetTwoSided(bool in) { twoSided = in; }
	bool IsTwoSided() const { return twoSided; }
	void SetVertexAnim(bool in) { vertAnimation = in; }
	void SetTerrainBlend(bool in) { terrainBlend = in; }
	void SetCinematicMode(bool in) { cinematicMode = in; lastCamPos = { FLT_MAX, FLT_MAX, FLT_MAX }; }

private:
	Mesh* mesh = nullptr;
	vector<shared_ptr<GameObject>> objects;
	vector<CachedInstanceData> cachedData;
	unique_ptr<UploadBuffer> instanceBuffer;
	unique_ptr<UploadBuffer> fullInstanceBuffer;
	vector<XMMATRIX> visibleTransforms;
	vector<XMMATRIX> shadowTransforms;
	UINT visibleCount = 0;
	UINT shadowCount = 0;
	bool castShadow = true;
	bool twoSided = false;
	bool vertAnimation = false;
	bool terrainBlend = false;
	bool cinematicMode = false;

	XMFLOAT3 lastCamPos = { FLT_MAX, FLT_MAX, FLT_MAX };
	static constexpr float UPDATE_THRESHOLD_SQ = 0.015625;  // 0.125^2

	vector<unique_ptr<UploadBuffer>> objectCBs;
};