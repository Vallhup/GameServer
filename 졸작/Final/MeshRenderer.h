#pragma once
#include "Component.h"
#include "Importer.h"

class VertexIndexBuffer;
class Material;
class UploadBuffer;

class MeshRenderer : public Component
{
public:
	MeshRenderer();
	~MeshRenderer();

	void Update(float deltaTime) override;
	void Render();
	void RenderInstanced(UINT instanceCount, UploadBuffer* instancedBuffer);
	void SetMesh(const wstring& path);

	void ReleaseUploadBuffers();

private:
	unique_ptr<VertexIndexBuffer> vertexIndexBuffer;  
	shared_ptr<Material> material;				// 단일 material
	
	vector<shared_ptr<Material>> materials;		// 다중 material
	vector<SubMeshInfo> subMeshes;				// 서브메시 정보 (다중 머티리얼용)

	bool visible = true;

	UINT myID;
	static UINT idCounter;
};

