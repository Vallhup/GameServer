#pragma once

class Mesh;
class GameObject;
class DX12Core;
class SceneRenderer;

struct CachedInstanceData
{
	XMMATRIX worldMatrix;
	XMFLOAT3 position;
	BoundingBox boundingBox;
	float cullDistance;
	bool needDistanceCull;
};

class InstancingBatch
{
public:
	void Initialize(Mesh* m);
	void AddObject(shared_ptr<GameObject> obj);
	void BuildBuffers(DX12Core& core);
	void Update(const BoundingFrustum& frustum, const XMVECTOR& camPos);
	void Render(DX12Core& core, SceneRenderer* renderer);
	void RenderShadow(DX12Core& core, SceneRenderer* renderer);
	void Clear();

	const vector<shared_ptr<GameObject>>& GetObjects() const { return objects; }

	D3D12_GPU_VIRTUAL_ADDRESS GetCBAddress(size_t idx) const { return objectCBs[idx]->GetGPUVirtualAddress(); }
	void SetCastShadow(bool in) { castShadow = in; }
	void SetTwoSided(bool in) { twoSided = in; }
	bool IsTwoSided() const { return twoSided; }

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

	XMFLOAT3 lastCamPos = { FLT_MAX, FLT_MAX, FLT_MAX };
	static constexpr float UPDATE_THRESHOLD_SQ = 0.015625;  // 0.125^2

	vector<unique_ptr<UploadBuffer>> objectCBs;
};