#include "pch.h"
#include "StartScene.h"
#include "Camera.h"
#include "Importer.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "CommandQueue.h"
#include "UploadBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "Texture.h"

void StartScene::Release()
{
}

void StartScene::Reset()
{
}

const float* StartScene::GetBackgroundColor()
{
	return Colors::LightBlue;
}

void StartScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    GET(Camera).Initialize();
}

void StartScene::UpdateLogic(const float deltaTime)
{
	GET(Camera).Update(deltaTime);
}

void StartScene::RenderScene()
{
    static Importer importer;
    static unique_ptr<VertexIndexBuffer> fbxMesh;

    if (!fbxMesh) {  
        if (importer.LoadModel(L"../FBXOutput/knight_Tpose")) {
            const MeshData& mesh = importer.GetMesh();

            fbxMesh = make_unique<VertexIndexBuffer>();
            fbxMesh->Initialize(
                GET(DX12Graphics).GetDevice()->GetDevice().Get(),
                GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(),
                mesh.vertices,
                mesh.indices
            );

            OutputDebugStringA("FBX Mesh created for rendering!\n");
        }
    }

    if (fbxMesh) {
        XMMATRIX worldMatrix = XMMatrixScaling(0.1f, 0.1f, 0.1f) *  // 크기
            XMMatrixTranslation(0.0f, 0.0f, 0.0f);   // 위치

        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(worldMatrix);
        objConstants.useTexture = 0;
        objConstants.heightScale = 1.0f;
        GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);

        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetPipelineState(GET(DX12Graphics).GetShader()->GetTransparentPSO());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

        ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootDescriptorTable(2, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
        GET(DX12Graphics).GetCmdQueue()->GetCmdList()->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetGroundTexture()->GetSRV());

        fbxMesh->Bind(GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get());
        fbxMesh->Draw(GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get());
    }
}

const GameObject* StartScene::GetWorld() const
{
	return nullptr;
}

int StartScene::GetSceneWidth() const
{
	return 0;
}
