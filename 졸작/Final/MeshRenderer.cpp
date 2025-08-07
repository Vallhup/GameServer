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

MeshRenderer::MeshRenderer() = default;
MeshRenderer::~MeshRenderer() = default;

void MeshRenderer::Update(float deltaTime)
{
    //OutputDebugStringA("Renderer's Update È£Ãâ!!\n");
}

void MeshRenderer::Render()
{
    if (!visible || !vertexIndexBuffer) return;

    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixTranspose(world);
    objConstants.useTexture = 0;
    objConstants.heightScale = 1.0f;
    GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);

    auto cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
    cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetTransparentPSO());
    cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(2, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
    cmdList->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetGroundTexture()->GetSRV());

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

		OutputDebugStringA("FBX Mesh created for rendering!\n");
	}
	else
		OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}
