#pragma once
#include "Component.h"
#include "Importer.h"

class DX12Core;
class VertexIndexBuffer;
class Material;

struct ObjectConstants;

class MeshRenderer : public Component
{
public:
	MeshRenderer();
	~MeshRenderer();

	void RenderForward(DX12Core& core);
	void RenderDeferred(DX12Core& core);
	void RenderShadow(DX12Core& core);

	void RenderInstanced(DX12Core& core, UINT instanceCount, UploadBuffer* instancedBuffer);

	void SetMesh(DX12Core& core, const wstring& path);

	void ReleaseUploadBuffers();
	void DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData> mats);

private:
	void InitializeObjectBuffer(ID3D12Device* device);

	void RenderSingleMaterialForwardOnly(DX12Core& core, const XMMATRIX& world);
	void RenderMultiMaterialForwardOnly(DX12Core& core, const XMMATRIX& world);
	void RenderSingleMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world);
	void RenderMultiMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world);

	void SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer = nullptr);
	void SetSingleMaterial(DX12Core& core, const vector<MaterialData> mats);
	void SetMultiMaterials(DX12Core& core, const vector<MaterialData> mats);
	ObjectConstants SetObjectConstantState(const XMMATRIX& world, int hasTexture, int doInstancing, UINT matIndex);

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;  

	shared_ptr<Material> material;				
	vector<shared_ptr<Material>> materials;		
	vector<SubMeshInfo> subMeshes;				

	vector<MaterialData> originalMaterialData;

	bool visible = true;

	UINT myID;
	static UINT idCounter;

	unique_ptr<UploadBuffer> objectCB;

	static constexpr size_t MAX_SUBMESH_COUNT = 10;
	static constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;
};

