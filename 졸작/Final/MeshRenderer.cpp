#include "pch.h"
#include "MeshRenderer.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Texture.h"
#include "GameObject.h"
#include "Transform.h"
#include "Material.h"
#include "VertexIndexBuffer.h"
#include "Animator.h"
#include "ResourceManager.h"

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

void MeshRenderer::Render(DX12Core& core)
{
    if (!visible || !vertexIndexBuffer) return;
    
    auto animator = GetGameObject()->GetComponent<Animator>();
    if (animator) {
        animator->ExecuteComputeShader(core);
    }
    
    /*auto cmdList = core.GetGraphicsCmdList();
    SetupRenderingState(core);

    if (animator) {
        cmdList->SetGraphicsRootShaderResourceView(8, animator->GetFinalBuffer()->GetGPUVirtualAddress());
    }

    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    if (!materials.empty()) {
        RenderMultiMaterial(core, world);
    }
    else if (material) {
        RenderSingleMaterial(core, world);
    }*/

    // 아래는 GBuffer test
    RenderToGBuffer(core);
}

void MeshRenderer::RenderForward(DX12Core& core)
{
    if (!visible || !vertexIndexBuffer) return;

    auto cmdList = core.GetGraphicsCmdList();
    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    cmdList->SetPipelineState(core.GetShader()->GetTransparentPSO());
    SetupRenderingState(core);

    if (auto animator = GetGameObject()->GetComponent<Animator>()) {
        cmdList->SetGraphicsRootShaderResourceView(8, animator->GetFinalBuffer()->GetGPUVirtualAddress());
    }

    if (!materials.empty()) {   // 많은 머티리얼 중 투명 값이 있는 머티리얼만 렌더링
        RenderMultiMaterialForwardOnly(core, world);
    }
}

void MeshRenderer::RenderDeferred(DX12Core& core)
{
    if (!visible || !vertexIndexBuffer) return;

    auto animator = GetGameObject()->GetComponent<Animator>();
    if (animator) {
        animator->ExecuteComputeShader(core);
    }

    auto cmdList = core.GetGraphicsCmdList();
    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    cmdList->SetPipelineState(core.GetShader()->GetGBufferPSO());
    SetupRenderingState(core);

    if (animator) {
        cmdList->SetGraphicsRootShaderResourceView(8,
            animator->GetFinalBuffer()->GetGPUVirtualAddress());
    }

    if (!materials.empty()) {
        RenderMultiMaterialDeferredOnly(core, world);
    }
    else if (material) {
        RenderSingleMaterialToGBuffer(core, world);
    }
}

void MeshRenderer::RenderToGBuffer(DX12Core& core)
{
    auto cmdList = core.GetGraphicsCmdList();
    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    // *** 핵심: G-Buffer PSO 사용! ***
    cmdList->SetPipelineState(core.GetShader()->GetGBufferPSO());

    SetupRenderingState(core);

    if (auto animator = GetGameObject()->GetComponent<Animator>()) {
        cmdList->SetGraphicsRootShaderResourceView(8,
            animator->GetFinalBuffer()->GetGPUVirtualAddress());
    }

    if (!materials.empty()) {
        // 멀티 머티리얼인 경우
        RenderMultiMaterialDeferredOnly(core, world);
    }
    else if (material) {
        // 단일 머티리얼인 경우
        RenderSingleMaterialToGBuffer(core, world);
    }
}

void MeshRenderer::RenderInstanced(DX12Core& core, UINT instanceCount, UploadBuffer* instanceBuffer)
{
    if (!visible || !vertexIndexBuffer || !instanceBuffer) return;

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixIdentity();  // 사용하지 않음
    objConstants.useTexture = (material != nullptr) ? 1 : 0;
    objConstants.useInstancing = 1;  // 인스턴싱 사용
    objConstants.materialIndex = material ? material->GetMaterialIndex() : 0xFFFFFFFF;

    core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);

    auto cmdList = core.GetGraphicsCmdList();
    SetupRenderingState(core, instanceBuffer);

    cmdList->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress());

    vertexIndexBuffer->Bind(cmdList);
    vertexIndexBuffer->DrawInstanced(cmdList, instanceCount);  
}

void MeshRenderer::RenderSingleMaterial(DX12Core& core, const XMMATRIX& world)
{
    auto cmdList = core.GetGraphicsCmdList();

    cmdList->SetPipelineState(core.GetShader()->GetOpaquePSO());

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixTranspose(world);
    objConstants.useTexture = 1;
    objConstants.useInstancing = 0;
    objConstants.hasAlpha = 0;
    objConstants.materialIndex = material->GetMaterialIndex();

    UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT offset = myID * cbSize;
    core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), offset);

    core.GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress() + offset);

    vertexIndexBuffer->Bind(core.GetGraphicsCmdList());
    vertexIndexBuffer->Draw(core.GetGraphicsCmdList());
}

void MeshRenderer::RenderMultiMaterial(DX12Core& core, const XMMATRIX& world)
{
    UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
    auto cmdList = core.GetGraphicsCmdList();

    cmdList->SetPipelineState(core.GetShader()->GetOpaquePSO());

    for (size_t i = 0; i < subMeshes.size(); ++i) {
        bool hasAlphaTexture = !originalMaterialData[i].alphaTexPath.empty();
        if (hasAlphaTexture) continue; 

        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(world);
        objConstants.useTexture = 1;
        objConstants.useInstancing = 0;
        objConstants.hasAlpha = 0;
        objConstants.materialIndex = materials[i]->GetMaterialIndex();

        UINT materialOffset = (myID * 5 + i) * cbSize;
        core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), materialOffset);
        cmdList->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress() + materialOffset);

        vertexIndexBuffer->Bind(cmdList);
        vertexIndexBuffer->DrawIndexed(cmdList, subMeshes[i].indexCount, subMeshes[i].startIndex);
    }

    cmdList->SetPipelineState(core.GetShader()->GetTransparentPSO());

    for (size_t i = 0; i < subMeshes.size(); ++i) {
        bool hasAlphaTexture = !originalMaterialData[i].alphaTexPath.empty();
        if (!hasAlphaTexture) continue; 

        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(world);
        objConstants.useTexture = 1;
        objConstants.useInstancing = 0;
        objConstants.hasAlpha = 1;
        objConstants.materialIndex = materials[i]->GetMaterialIndex();

        UINT materialOffset = (myID * 5 + i) * cbSize;
        core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), materialOffset);
        cmdList->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress() + materialOffset);

        vertexIndexBuffer->Bind(cmdList);
        vertexIndexBuffer->DrawIndexed(cmdList, subMeshes[i].indexCount, subMeshes[i].startIndex);
    }
}

void MeshRenderer::RenderMultiMaterialForwardOnly(DX12Core& core, const XMMATRIX& world)
{
    UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
    auto cmdList = core.GetGraphicsCmdList();

    for (size_t i = 0; i < subMeshes.size(); ++i) {
        bool hasAlphaTexture = !originalMaterialData[i].alphaTexPath.empty();
        if (!hasAlphaTexture) continue;

        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(world);
        objConstants.useTexture = 1;
        objConstants.useInstancing = 0;
        objConstants.hasAlpha = 0;
        objConstants.materialIndex = materials[i]->GetMaterialIndex();

        UINT materialOffset = (myID * 5 + i) * cbSize;
        core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), materialOffset);
        cmdList->SetGraphicsRootConstantBufferView(1,
            core.GetSceneCB()->GetGPUVirtualAddress() + materialOffset);

        vertexIndexBuffer->Bind(cmdList);
        vertexIndexBuffer->DrawIndexed(cmdList,
            subMeshes[i].indexCount,
            subMeshes[i].startIndex);
    }
}

void MeshRenderer::RenderSingleMaterialToGBuffer(DX12Core& core, const XMMATRIX& world)
{
    auto cmdList = core.GetGraphicsCmdList();

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixTranspose(world);
    objConstants.useTexture = 1;
    objConstants.useInstancing = 0;
    objConstants.hasAlpha = 0;
    objConstants.materialIndex = material->GetMaterialIndex();

    UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT offset = myID * cbSize;
    core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), offset);
    cmdList->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress() + offset);

    vertexIndexBuffer->Bind(core.GetGraphicsCmdList());
    vertexIndexBuffer->Draw(core.GetGraphicsCmdList());
}

void MeshRenderer::RenderMultiMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world)
{
    UINT cbSize = (sizeof(ObjectConstants) + 255) & ~255;
    auto cmdList = core.GetGraphicsCmdList();

    for (size_t i = 0; i < subMeshes.size(); ++i) {
        bool hasAlphaTexture = !originalMaterialData[i].alphaTexPath.empty();
        if (hasAlphaTexture) continue;

        ObjectConstants objConstants = {};
        objConstants.world = XMMatrixTranspose(world);
        objConstants.useTexture = 1;
        objConstants.useInstancing = 0;
        objConstants.hasAlpha = 0;
        objConstants.materialIndex = materials[i]->GetMaterialIndex();

        UINT materialOffset = (myID * 5 + i) * cbSize;
        core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), materialOffset);
        cmdList->SetGraphicsRootConstantBufferView(1,
            core.GetSceneCB()->GetGPUVirtualAddress() + materialOffset);

        vertexIndexBuffer->Bind(cmdList);
        vertexIndexBuffer->DrawIndexed(cmdList,
            subMeshes[i].indexCount,
            subMeshes[i].startIndex);
    }
}

void MeshRenderer::SetMesh(DX12Core& core, const wstring& path)
{
    auto startTime = chrono::high_resolution_clock::now();

    auto cachedMesh = GET(ResourceManager).GetCachedMesh(path);
    if (cachedMesh) {
        vertexIndexBuffer = cachedMesh->vertexIndexBuffer;
        subMeshes = cachedMesh->subMeshes;
        originalMaterialData = cachedMesh->originalMaterialData;
        
        if (originalMaterialData.size() > 1) {
            SetMultiMaterials(core, originalMaterialData);
        }
        else {
            SetSingleMaterial(core, originalMaterialData);
        }

        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator && cachedMesh->hasAnimation) {
            animator->SetAnimationData(core, cachedMesh->animationClips);
            animator->SetSkeletonData(cachedMesh->skeletonData);
        }

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE HIT - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());
        return;
    }

	Importer importer;
	if (importer.LoadModel(path))
	{
		const MeshData& mesh = importer.GetMesh();
		vertexIndexBuffer = make_shared<VertexIndexBuffer>();
		vertexIndexBuffer->Initialize(
            core.GetDevice(),
            core.GetGraphicsCmdList(),
			mesh.vertices,
			mesh.indices
		);

        const auto& mats = importer.GetMaterials();

        DebugMaterialInfo(mesh, mats);

        if (mesh.subMeshes.size() > 1)
        {
            subMeshes = mesh.subMeshes;
            SetMultiMaterials(core, mats);
        }
        else
        {
            SetSingleMaterial(core, mats);
        }

        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator) {
            animator->LoadAnimationFromImporter(core, importer);
        }

        GET(ResourceManager).CacheMesh(path, vertexIndexBuffer, subMeshes, originalMaterialData, 
            mesh.hasAnimation, importer.GetAnimations(), importer.GetSkeleton());

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE MISS - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());

		OutputDebugStringA("FBX Mesh created for rendering!\n");
	}
	else
		OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}

// MeshRenderer::SetupRenderingState에서 조명 관련 코드 전부 삭제
void MeshRenderer::SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer)
{
    ID3D12GraphicsCommandList* cmdList = core.GetGraphicsCmdList();

    cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
    Material::BindBindlessResources(cmdList);
    cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

    // 조명 설정 코드 전부 삭제!
    
    if (instanceBuffer) {
        cmdList->SetGraphicsRootShaderResourceView(9, instanceBuffer->GetGPUVirtualAddress());
    }
}

void MeshRenderer::SetSingleMaterial(DX12Core& core, const vector<MaterialData> mats)
{
    material = make_shared<Material>();
    material->LoadFromMaterialData(
        core.GetDevice(),
        core.GetGraphicsCmdList(),
        mats[0]
    );
}

void MeshRenderer::SetMultiMaterials(DX12Core& core, const vector<MaterialData> mats)
{
    originalMaterialData = mats;

    for (const auto& matData : mats)
    {
        auto mat = make_shared<Material>();
        mat->LoadFromMaterialData(
            core.GetDevice(),
            core.GetGraphicsCmdList(),
            matData
        );

        materials.push_back(mat);
    }
}

void MeshRenderer::ReleaseUploadBuffers()
{
    if (vertexIndexBuffer) {
        vertexIndexBuffer->ReleaseUploadBuffers();
    }

    // 애니메이션 관련 Uploadbuffers는 지속적인 업데이트를 위해 해제 안하는게 맞음.
}

void MeshRenderer::DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData> mats)
{
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
}
