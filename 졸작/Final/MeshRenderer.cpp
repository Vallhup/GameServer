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
#include "UploadBuffer.h"
#include "UAVBuffer.h"

UINT MeshRenderer::idCounter = 0;

MeshRenderer::MeshRenderer()
{
    myID = idCounter++;
    objectCB = nullptr;
}

MeshRenderer::~MeshRenderer() = default;

void MeshRenderer::InitializeObjectBuffer(ID3D12Device* device)
{
    if (!objectCB) {
        size_t bufferSize = CONSTANT_BUFFER_ALIGNMENT * MAX_SUBMESH_COUNT;   // subMesh 최대 개수 10개 안넘을듯?
        objectCB = make_unique<UploadBuffer>();
        objectCB->Initialize(device, bufferSize);

        OutputDebugStringA(("MeshRenderer " + to_string(myID) + " ObjectBuffer initialized\n").c_str());
    }
}

void MeshRenderer::RenderForward(DX12Core& core)
{
    if (!visible || !vertexIndexBuffer) return;

    auto cmdList = core.GetGraphicsCmdList();
    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Transparent));
    SetupRenderingState(core);

    if (auto animator = GetGameObject()->GetComponent<Animator>()) {
        cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());      // 레지 넘버링 부분
    }

    if (!materials.empty()) {   
        RenderMultiMaterialForwardOnly(core, world);
    }
    else if (material) {
        RenderSingleMaterialForwardOnly(core, world);
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

    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBuffer));
    SetupRenderingState(core);

    if (animator) {
        cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());      // 레지 넘버링 부분
    }

    if (!materials.empty()) {
        RenderMultiMaterialDeferredOnly(core, world);
    }
    else if (material) {
        RenderSingleMaterialDeferredOnly(core, world);
    }
}

void MeshRenderer::RenderShadow(DX12Core& core)
{
    if (!visible || !vertexIndexBuffer) return;
    if (!objectCB) {
        InitializeObjectBuffer(core.GetDevice());
    }

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Shadow));
    auto transform = GetGameObject()->GetComponent<Transform>();
    XMMATRIX world = transform->GetWorldMatrix();

    ObjectConstants objConstants = SetObjectConstantState(XMMatrixTranspose(world), 0, 0, 0);

    objectCB->CopyData(&objConstants, sizeof(ObjectConstants));
    cmdList->SetGraphicsRootConstantBufferView(1, objectCB->GetGPUVirtualAddress());

    if (auto animator = GetGameObject()->GetComponent<Animator>())
        cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());

    vertexIndexBuffer->Bind(cmdList);
    vertexIndexBuffer->Draw(cmdList);
}

void MeshRenderer::RenderInstanced(DX12Core& core, UINT instanceCount, UploadBuffer* instanceBuffer)
{
    if (!visible || !vertexIndexBuffer || !instanceBuffer) return;

    ObjectConstants objConstants = SetObjectConstantState(XMMatrixIdentity(),
        (material != nullptr) ? 1 : 0, 1, material ? material->GetMaterialIndex() : 0xFFFFFFFF);

    core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants), 0);

    auto cmdList = core.GetGraphicsCmdList();
    SetupRenderingState(core, instanceBuffer);

    cmdList->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress());       // 레지 넘버링 부분

    vertexIndexBuffer->Bind(cmdList);
    vertexIndexBuffer->DrawInstanced(cmdList, instanceCount);
}

void MeshRenderer::RenderSingleMaterialForwardOnly(DX12Core& core, const XMMATRIX& world)
{
    if (!objectCB) {
        InitializeObjectBuffer(core.GetDevice());
    }

    auto cmdList = core.GetGraphicsCmdList();

    ObjectConstants objConstants = SetObjectConstantState(XMMatrixTranspose(world),
        1, 0, material->GetMaterialIndex());

    objectCB->CopyData(&objConstants, sizeof(ObjectConstants));
    cmdList->SetGraphicsRootConstantBufferView(1, objectCB->GetGPUVirtualAddress());        // 레지 넘버링 부분

    vertexIndexBuffer->Bind(core.GetGraphicsCmdList());
    vertexIndexBuffer->Draw(core.GetGraphicsCmdList());
}

void MeshRenderer::RenderMultiMaterialForwardOnly(DX12Core& core, const XMMATRIX& world)
{
    if (!objectCB) {
        InitializeObjectBuffer(core.GetDevice());
    }

    auto cmdList = core.GetGraphicsCmdList();

    for (size_t i = 0; i < subMeshes.size(); ++i) {
        bool hasAlphaTexture = !originalMaterialData[i].alphaTexPath.empty();
        if (!hasAlphaTexture) continue;

        ObjectConstants objConstants = SetObjectConstantState(XMMatrixTranspose(world),
            1, 0, materials[i]->GetMaterialIndex());

        size_t offset = i * CONSTANT_BUFFER_ALIGNMENT;

        objectCB->CopyData(&objConstants, sizeof(ObjectConstants), offset);
        cmdList->SetGraphicsRootConstantBufferView(1, objectCB->GetGPUVirtualAddress() + offset);       // 레지 넘버링 부분

        vertexIndexBuffer->Bind(cmdList);
        vertexIndexBuffer->DrawIndexed(cmdList,
            subMeshes[i].indexCount,
            subMeshes[i].startIndex);
    }
}

void MeshRenderer::RenderSingleMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world)
{
    if (!objectCB) {
        InitializeObjectBuffer(core.GetDevice());
    }

    auto cmdList = core.GetGraphicsCmdList();

    ObjectConstants objConstants = SetObjectConstantState(XMMatrixTranspose(world),
        1, 0, material->GetMaterialIndex());

    objectCB->CopyData(&objConstants, sizeof(ObjectConstants));
    cmdList->SetGraphicsRootConstantBufferView(1, objectCB->GetGPUVirtualAddress());        // 레지 넘버링 부분

    vertexIndexBuffer->Bind(core.GetGraphicsCmdList());
    vertexIndexBuffer->Draw(core.GetGraphicsCmdList());
}

void MeshRenderer::RenderMultiMaterialDeferredOnly(DX12Core& core, const XMMATRIX& world)
{
    if (!objectCB) {
        InitializeObjectBuffer(core.GetDevice());
    }

    auto cmdList = core.GetGraphicsCmdList();

    for (size_t i = 0; i < subMeshes.size(); ++i) {
        bool hasAlphaTexture = !originalMaterialData[i].alphaTexPath.empty();
        if (hasAlphaTexture) continue;

        ObjectConstants objConstants = SetObjectConstantState(XMMatrixTranspose(world), 
            1, 0, materials[i]->GetMaterialIndex());

        size_t offset = i * CONSTANT_BUFFER_ALIGNMENT;

        objectCB->CopyData(&objConstants, sizeof(ObjectConstants), offset);
        cmdList->SetGraphicsRootConstantBufferView(1, objectCB->GetGPUVirtualAddress() + offset);       // 레지 넘버링 부분

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

        materials.clear();    // 중복 방지
        material.reset();

        const auto& matIdx = cachedMesh->materialIndices;
        if (matIdx.size() > 1) {
            materials.reserve(matIdx.size());
            for (UINT idx : matIdx)
                materials.push_back(Material::FromExistingIndex(idx));
        }
        else if (matIdx.size() == 1) {
            material = Material::FromExistingIndex(matIdx[0]);
        }

        subMeshes = cachedMesh->subMeshes;
        originalMaterialData = cachedMesh->originalMaterialData;

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

        vector<UINT> matIndices;
        if (!materials.empty()) {
            matIndices.reserve(materials.size());
            for (auto& m : materials)
                matIndices.push_back(m->GetMaterialIndex());
        }
        else if (material) {
            matIndices = { material->GetMaterialIndex() };
        }

        GET(ResourceManager).CacheMesh(path, vertexIndexBuffer, matIndices, subMeshes, originalMaterialData,
            mesh.hasAnimation, importer.GetAnimations(), importer.GetSkeleton());

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE MISS - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());

        OutputDebugStringA("FBX Mesh created for rendering!\n");
    }
    else
        OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}

void MeshRenderer::SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer)
{
    ID3D12GraphicsCommandList* cmdList = core.GetGraphicsCmdList();

    cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
    Material::BindBindlessResources(cmdList);
    cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());       // 레지 넘버링 부분

    if (instanceBuffer) {
        cmdList->SetGraphicsRootShaderResourceView(12, instanceBuffer->GetGPUVirtualAddress());     // 레지 넘버링 부분
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

ObjectConstants MeshRenderer::SetObjectConstantState(const XMMATRIX& world, int hasTexture, int doInstancing, UINT matIndex)
{
    ObjectConstants objConstants = {};
    objConstants.world = world;
    objConstants.useTexture = hasTexture;
    objConstants.useInstancing = doInstancing;
    objConstants.materialIndex = matIndex;

    return objConstants;
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