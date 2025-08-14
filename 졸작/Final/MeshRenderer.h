#pragma once
#include "Component.h"
#include "Importer.h"
#include "UploadBuffer.h"

class VertexIndexBuffer;
class Material;

class MeshRenderer : public Component
{
public:
	MeshRenderer();
	~MeshRenderer();

	void Update(float deltaTime) override;
	void Render();
	void RenderInstanced(UINT instanceCount, UploadBuffer* instancedBuffer);
	void RenderSingleMaterial(ID3D12GraphicsCommandList* cmdList, const XMMATRIX& world);
	void RenderMultiMaterial(ID3D12GraphicsCommandList* cmdList, const XMMATRIX& world);

	void SetMesh(const wstring& path);
	void SetupRenderingState(ID3D12GraphicsCommandList* cmdList, UploadBuffer* instanceBuffer = nullptr);
	void SetSingleMaterial(const vector<MaterialData> mats);
	void SetMultiMaterials(const vector<MaterialData> mats);

	void ReleaseUploadBuffers();

	void DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData> mats);

private:
	unique_ptr<VertexIndexBuffer> vertexIndexBuffer;  

	shared_ptr<Material> material;				// 단일 material
	vector<shared_ptr<Material>> materials;		// 다중 material
	vector<SubMeshInfo> subMeshes;				// 서브메시 정보 (다중 머티리얼용)

	bool visible = true;

	UINT myID;
	static UINT idCounter;
};

