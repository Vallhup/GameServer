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

	void InitializeObjectBuffer(ID3D12Device* device);

	void RenderForward(DX12Core& core);
	void RenderDeferred(DX12Core& core);

	// Instancing 전용 함수는 나중에 사용할 수도 있을 가능성이 높아서 그냥 냅둠
	void RenderInstanced(DX12Core& core, UINT instanceCount, UploadBuffer* instancedBuffer);

	void RenderSingleMaterialForwardOnly(DX12Core& core, const XMMATRIX& world);
	void RenderMultiMaterialForwardOnly(DX12Core& core, const XMMATRIX& world);

	void RenderSingleMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world);
	void RenderMultiMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world);

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

	unique_ptr<UploadBuffer> objectCB;

	static constexpr size_t MAX_SUBMESH_COUNT = 10;
	static constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;
};

