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

UINT MeshRenderer::idCounter = 0;

MeshRenderer::MeshRenderer()
{
    myID = idCounter++;
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
    objConstants.useInstancing = 0;  // 일반 렌더링

    UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT offset = myID * cbSize;

    GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), offset);

    auto cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
    cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());
    cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress() + offset);

    cmdList->SetGraphicsRootDescriptorTable(2, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
    cmdList->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetGroundTexture()->GetSRV());

    if (material)
    {
        material->BindToShader(cmdList, 4);
    }

    vertexIndexBuffer->Bind(cmdList);
    vertexIndexBuffer->Draw(cmdList);
}

void MeshRenderer::RenderInstanced(UINT instanceCount, UploadBuffer* instanceBuffer)
{
    if (!visible || !vertexIndexBuffer || !instanceBuffer) return;

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixIdentity();  // 사용하지 않음
    objConstants.useTexture = (material != nullptr) ? 1 : 0;
    objConstants.heightScale = 1.0f;
    objConstants.useInstancing = 1;  // 인스턴싱 사용

    GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);

    auto cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
    cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());
    cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Frame Constants (View/Projection)
    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(5, instanceBuffer->GetGPUVirtualAddress());

    // 텍스처들
    cmdList->SetGraphicsRootDescriptorTable(2, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
    cmdList->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetGroundTexture()->GetSRV());

    if (material)
    {
        material->BindToShader(cmdList, 4);
    }

    vertexIndexBuffer->Bind(cmdList);
    vertexIndexBuffer->DrawInstanced(cmdList, instanceCount);  // 새 메서드 필요
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

        // 디버깅 추가
        OutputDebugStringA(("Total materials found: " + to_string(materials.size()) + "\n").c_str());

        for (size_t i = 0; i < materials.size(); ++i) {
            string msg = "Material[" + to_string(i) + "]: " + materials[i].name + "\n";
            OutputDebugStringA(msg.c_str());

            OutputDebugStringA(("  BaseColor: " + materials[i].baseColorTexPath + "\n").c_str());
            OutputDebugStringA(("  Normal: " + materials[i].normalTexPath + "\n").c_str());
            OutputDebugStringA(("  Roughness: " + materials[i].roughnessTexPath + "\n").c_str());
            OutputDebugStringA(("  Metallic: " + materials[i].metallicTexPath + "\n").c_str());
            OutputDebugStringA(("  Height: " + materials[i].heightTexPath + "\n").c_str());
            OutputDebugStringA(("  Alpha: " + materials[i].alphaTexPath + "\n").c_str());
            OutputDebugStringA(("  Emission: " + materials[i].emissionTexPath + "\n").c_str());
            OutputDebugStringA(("  AO: " + materials[i].aoTexPath + "\n").c_str());
        }

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
