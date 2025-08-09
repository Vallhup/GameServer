#include "pch.h"
#include "MeshRenderer.h"
#include "Importer.h"
#include "VertexIndexBuffer.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "CommandQueue.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "Texture.h"
#include "GameObject.h"
#include "Transform.h"
#include "Material.h"

int MeshRenderer::nextInstanceId = 0;

MeshRenderer::MeshRenderer()
{
    instanceId = nextInstanceId++;
}

MeshRenderer::~MeshRenderer() = default;

void MeshRenderer::Update(float deltaTime)
{
    //OutputDebugStringA("Renderer's Update 호출!!\n");
}

void MeshRenderer::Render()
{
    if (!visible || !vertexIndexBuffer) return;

    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixTranspose(world);
    objConstants.useTexture = (material != nullptr) ? 1 : 0;
    objConstants.heightScale = 1.0f;

    size_t alignedOffset = instanceId * 256;  // 각 인스턴스마다 고유 offset
    GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), alignedOffset);
    //GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);  // 하나 일때



    auto cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
    cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());
    cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
    
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress() + alignedOffset;
    cmdList->SetGraphicsRootConstantBufferView(1, cbAddress);

    // 하나 일 때
    //cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress());
    
    cmdList->SetGraphicsRootDescriptorTable(2, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
    cmdList->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetGroundTexture()->GetSRV());

    if (material)
    {
        material->BindToShader(cmdList, 4);
    }

    vertexIndexBuffer->Bind(cmdList); 
    vertexIndexBuffer->Draw(cmdList); 
}

void MeshRenderer::SetMesh(const wstring& path)
{
	Importer importer;

	if (importer.LoadModel(path))
	{
		const MeshData& mesh = importer.GetMesh();
		vertexIndexBuffer = make_unique<VertexIndexBuffer>();
		vertexIndexBuffer->Initialize(
			GET(DX12Graphics).GetDevice()->GetDevice().Get(),
			GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(),
			mesh.vertices,
			mesh.indices
		);

        const auto& materials = importer.GetMaterials();
        if (!materials.empty())
        {
            material = make_shared<Material>();
            material->LoadFromMaterialData(
                GET(DX12Graphics).GetDevice()->GetDevice().Get(),
                GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(),
                materials[0],
                GET(DX12Graphics).GetDescHeap()
            );
        }

		OutputDebugStringA("FBX Mesh created for rendering!\n");
	}
	else
		OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}
