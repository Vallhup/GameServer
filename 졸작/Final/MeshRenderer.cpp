#include "pch.h"
#include "MeshRenderer.h"
#include "VertexIndexBuffer.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "CommandQueue.h"
#include "Shader.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "Texture.h"
#include "GameObject.h"
#include "Transform.h"
#include "Material.h"
#include "Animator.h"

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
    
    // 1. Animator 컴포넌트 확인
    auto animator = GetGameObject()->GetComponent<Animator>();
    auto cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();

    // 2. 애니메이션이 있으면 Compute Shader 실행
    if (animator) {
        OutputDebugStringA(("Bone Count: " + to_string(animator->GetBoneCount()) + "\n").c_str());
        OutputDebugStringA(("Current Frame: " + to_string(animator->GetCurrentFrame()) + "\n").c_str());
        OutputDebugStringA(("Frame Ratio: " + to_string(animator->GetFrameRatio()) + "\n").c_str());

        // AnimationConstants 설정
        AnimationConstants animData = {};
        animData.boneCount = animator->GetBoneCount();
        animData.currentFrame = animator->GetCurrentFrame();
        animData.nextFrame = animator->GetNextFrame();
        animData.ratio = animator->GetFrameRatio();

        // Compute Shader 실행
        GET(DX12Graphics).GetAnimationCB()->CopyData(&animData, sizeof(AnimationConstants));

        cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetComputePSO());
        cmdList->SetComputeRootSignature(GET(DX12Graphics).GetRootSig()->Get());
        cmdList->SetComputeRootConstantBufferView(2, GET(DX12Graphics).GetAnimationCB()->GetGPUVirtualAddress());

        // BoneFrame, Offset 데이터 바인딩
        cmdList->SetComputeRootShaderResourceView(6, animator->GetBoneFrameBuffer()->GetGPUVirtualAddress());  // t10
        cmdList->SetComputeRootShaderResourceView(7, animator->GetOffsetBuffer()->GetGPUVirtualAddress());     // t11

        // Final 본 행렬 바인딩 (Compute Shader 출력)
        cmdList->SetComputeRootUnorderedAccessView(8, animator->GetFinalBuffer()->GetGPUVirtualAddress());     // u0

        UINT groupCount = (animData.boneCount + 255) / 256;  // 256으로 나눠서 올림
        cmdList->Dispatch(groupCount, 1, 1);
    }

    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();
    
    cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());
    cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { GET(DX12Graphics).GetDescHeap()->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
    cmdList->SetGraphicsRootDescriptorTable(4, GET(DX12Graphics).GetGroundTexture()->GetSRV());

    if (animator) {
        cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());
    }

    if (!materials.empty()) {
        // 다중 머티리얼 렌더링
        vertexIndexBuffer->Bind(cmdList);
        
        UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
        
        for (size_t i = 0; i < subMeshes.size(); ++i) {
            ObjectConstants objConstants = {};
            objConstants.world = XMMatrixTranspose(world);
            objConstants.useTexture = 1;
            objConstants.heightScale = 1.0f;
            objConstants.useInstancing = 0;
            
            const auto& matData = materials[i]->GetMaterialData();
            objConstants.hasAlpha = !matData.alphaTexPath.empty() ? 1 : 0;
            
            UINT materialOffset = (myID * 5 + i) * cbSize;  // 5는 최대 머티리얼 수
            GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), materialOffset);
            
            cmdList->SetGraphicsRootConstantBufferView(1, 
                GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress() + materialOffset);
            
            materials[i]->BindToShader(cmdList, 5);
            vertexIndexBuffer->DrawIndexed(cmdList,
                subMeshes[i].indexCount,
                subMeshes[i].startIndex);
        }
    }
    else if (material) {
        // 기존 단일 머티리얼 렌더링
        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(world);
        objConstants.useTexture = 1;
        objConstants.heightScale = 1.0f;
        objConstants.useInstancing = 0;
        objConstants.hasAlpha = 0;  // 단일 머티리얼은 Alpha 없음
        
        UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
        UINT offset = myID * cbSize;
        GET(DX12Graphics).GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), offset);
        
        cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress() + offset);
        
        material->BindToShader(cmdList, 5);
        vertexIndexBuffer->Bind(cmdList);
        vertexIndexBuffer->Draw(cmdList);
    }
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

    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, GET(DX12Graphics).GetSceneCB()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(9, instanceBuffer->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(3, GET(DX12Graphics).GetHeightMapTexture()->GetSRV());
    cmdList->SetGraphicsRootDescriptorTable(4, GET(DX12Graphics).GetGroundTexture()->GetSRV());

    if (material)
    {
        material->BindToShader(cmdList, 5);
    }

    vertexIndexBuffer->Bind(cmdList);
    vertexIndexBuffer->DrawInstanced(cmdList, instanceCount);  
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

        const auto& mats = importer.GetMaterials();

        OutputDebugStringA(("Total materials found: " + to_string(mats.size()) + "\n").c_str());

        for (size_t i = 0; i < mats.size(); ++i) {
            string msg = "Material[" + to_string(i) + "]: " + mats[i].name + "\n";
            OutputDebugStringA(msg.c_str());

            OutputDebugStringA(("  BaseColor: " + mats[i].baseColorTexPath + "\n").c_str());
            OutputDebugStringA(("  Normal: " + mats[i].normalTexPath + "\n").c_str());
            OutputDebugStringA(("  Roughness: " + mats[i].roughnessTexPath + "\n").c_str());
            OutputDebugStringA(("  Metallic: " + mats[i].metallicTexPath + "\n").c_str());
            OutputDebugStringA(("  Height: " + mats[i].heightTexPath + "\n").c_str());
            OutputDebugStringA(("  Alpha: " + mats[i].alphaTexPath + "\n").c_str());
            OutputDebugStringA(("  Emission: " + mats[i].emissionTexPath + "\n").c_str());
            OutputDebugStringA(("  AO: " + mats[i].aoTexPath + "\n").c_str());
        }

        OutputDebugStringA(("SubMesh count: " + to_string(mesh.subMeshes.size()) + "\n").c_str());

        for (size_t i = 0; i < mesh.subMeshes.size(); ++i) {
            string msg = "SubMesh[" + to_string(i) + "]: ";
            msg += "StartIndex=" + to_string(mesh.subMeshes[i].startIndex) + ", ";
            msg += "IndexCount=" + to_string(mesh.subMeshes[i].indexCount) + ", ";
            msg += "MaterialIndex=" + to_string(mesh.subMeshes[i].materialIndex) + "\n";
            OutputDebugStringA(msg.c_str());
        }

        if (mesh.subMeshes.size() > 1)
        {
            // 다중 Material 용
            subMeshes = mesh.subMeshes;
            for (const auto& matData : mats)
            {
                auto mat = make_shared<Material>();
                mat->LoadFromMaterialData(GET(DX12Graphics).GetDevice()->GetDevice().Get(),
                    GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(),
                    matData,
                    GET(DX12Graphics).GetDescHeap()
                );

                materials.push_back(mat);
            }
        }
        else
        {
            // 단일 Material 용
            material = make_shared<Material>();
            material->LoadFromMaterialData(
                GET(DX12Graphics).GetDevice()->GetDevice().Get(),
                GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(),
                mats[0],
                GET(DX12Graphics).GetDescHeap()
            );
        }

        // 애니메이션 데이터 전달 추가
        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator) {
            const auto& animations = importer.GetAnimations();
            const auto& skeleton = importer.GetSkeleton();

            if (!animations.empty()) {
                animator->SetAnimationData(animations);
                animator->SetSkeletonData(skeleton);
                OutputDebugStringA("Animation data loaded!\n");
            }
        }

		OutputDebugStringA("FBX Mesh created for rendering!\n");
	}
	else
		OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}

void MeshRenderer::ReleaseUploadBuffers()
{
    if (vertexIndexBuffer) {
        vertexIndexBuffer->ReleaseUploadBuffers();
    }
    if (material) {
        material->ReleaseUploadBuffers();
    }
    for (auto& mat : materials) {
        mat->ReleaseUploadBuffers();
    }
}