#pragma once

class Mesh;
class GameObject;
class DX12Core;
class SceneRenderer;

class InstancingBatch 
{
public:
	void Initialize(Mesh* m);
	void AddObject(shared_ptr<GameObject> obj);
	void BuildBuffers(DX12Core& core);
	void Update(const BoundingFrustum& frustum);
	void Render(DX12Core& core, SceneRenderer* renderer);
	void RenderShadow(DX12Core& core, SceneRenderer* renderer);
	void Clear();

	const vector<shared_ptr<GameObject>>& GetObjects() const { return objects; }

private:
	Mesh* mesh;
	vector<shared_ptr<GameObject>> objects;
	unique_ptr<UploadBuffer> instanceBuffer;
	unique_ptr<UploadBuffer> fullInstanceBuffer;
	UINT visibleCount = 0;
};