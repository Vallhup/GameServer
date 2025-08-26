#pragma once
#include "Component.h"
#include "Importer.h"

class DX12Core;
class VertexIndexBuffer;
class Material;
class UploadBuffer;

class MeshRenderer : public Component
{
public:
	MeshRenderer();
	~MeshRenderer();

	void Update(float deltaTime) override;
	void Render(DX12Core& core);
	void RenderToGBuffer(DX12Core& core);
	void RenderInstanced(DX12Core& core, UINT instanceCount, UploadBuffer* instancedBuffer);

	void RenderSingleMaterial(DX12Core& core, const XMMATRIX& world);
	void RenderMultiMaterial(DX12Core& core, const XMMATRIX& world);

	void RenderSingleMaterialToGBuffer(DX12Core& core, const XMMATRIX& world);
	void RenderMultiMaterialToGBuffer(DX12Core& core, const XMMATRIX& world);

	void SetMesh(DX12Core& core, const wstring& path);
	void SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer = nullptr);
	void SetSingleMaterial(DX12Core& core, const vector<MaterialData> mats);
	void SetMultiMaterials(DX12Core& core, const vector<MaterialData> mats);

	void ReleaseUploadBuffers();

	void DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData> mats);

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;  

	shared_ptr<Material> material;				// 단일 material
	vector<shared_ptr<Material>> materials;		// 다중 material
	vector<SubMeshInfo> subMeshes;				// 서브메시 정보 (다중 머티리얼용)

	vector<MaterialData> originalMaterialData;

	bool visible = true;

	UINT myID;
	static UINT idCounter;
};

