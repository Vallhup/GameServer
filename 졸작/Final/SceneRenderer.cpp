#include "pch.h"
#include "SceneRenderer.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Animator.h"
#include "Transform.h"
#include "Material.h"
#include "VertexIndexBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Camera.h"
#include "Terrain.h"
#include "Water.h"
#include "InstancingBatch.h"

void SceneRenderer::Initialize(ID3D12Device* device)
{
    objectCBPool = make_unique<UploadBuffer>();
    objectCBPool->Initialize(device, CONSTANT_BUFFER_ALIGNMENT * MAX_OBJECTS);
}

void SceneRenderer::BeginFrame()
{
    cbIndex = 0;
}

void SceneRenderer::RenderDeferred(DX12Core& core, const vector<shared_ptr<GameObject>>& objects, const Camera* cam)
{
    UINT startIndex = cbIndex;
    auto cmdList = core.GetGraphicsCmdList();

    for (const auto& obj : objects)
    {
        auto animator = obj->GetComponent<Animator>();
        if (animator)
            animator->ExecuteComputeShader(core);
    }

    SetupRenderingState(core);

    BoundingFrustum frustum;
    XMFLOAT3 camPos = {};

    if (cam) 
    {
        frustum = cam->GetViewFrustum();
        camPos = cam->GetPosition();
    }

    XMVECTOR camPosVec = XMLoadFloat3(&camPos);

    for (const auto& obj : objects)
    {
        if (obj->GetId() == -1) continue;
        if (cam && !obj->IsVisible(frustum, camPosVec)) continue;

        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->GetVertexIndexBuffer()) continue;

        if (cbIndex >= MAX_OBJECTS) {
            OutputDebugStringA("cbIndex Overflowed!!\n");
            break;
        }

        if (mesh->IsTwoSided())
            cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBufferNonCulling));
        else
            cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBuffer));

        auto animator = obj->GetComponent<Animator>();
        if (animator) {
            cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());
        }

        auto transform = obj->GetComponent<Transform>();
        XMMATRIX world = XMMatrixTranspose(transform->GetWorldMatrix());

        mesh->GetVertexIndexBuffer()->Bind(cmdList);

        if (mesh->HasMultiMaterial())
        {
            const auto& subMeshes = mesh->GetSubMeshes();
            const auto& materials = mesh->GetMaterials();
            const auto& originalData = mesh->GetOriginalMaterialData();

            for (size_t i = 0; i < subMeshes.size(); ++i)
            {
                bool hasAlpha = !originalData[i].alphaTexPath.empty();
                if (hasAlpha) continue;

                auto objConst = MakeObjectConstants(world, 1, 0, materials[i]->GetMaterialIndex());
                size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
                objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
                cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
                cbIndex++;

                mesh->GetVertexIndexBuffer()->DrawIndexed(cmdList, subMeshes[i].indexCount, subMeshes[i].startIndex);
            }
        }
        else
        {
            UINT matIndex = mesh->GetMaterial() ? mesh->GetMaterial()->GetMaterialIndex() : 0;
            auto objConst = MakeObjectConstants(world, 1, 0, matIndex);
            size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
            objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
            cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
            cbIndex++;

            mesh->GetVertexIndexBuffer()->Draw(cmdList);
        }
    }

    if (GetAsyncKeyState('P') & 0x8000)
    {
        string msg = "[Deferred Pass] Index: " + to_string(startIndex) + " ~ " + to_string(cbIndex)
            + " (Count: " + to_string(cbIndex - startIndex) + ")\n";
        OutputDebugStringA(msg.c_str());
    }
}

void SceneRenderer::RenderForward(DX12Core& core, const vector<shared_ptr<GameObject>>& objects, const Camera* cam)
{
    UINT startIndex = cbIndex;

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Transparent));
    SetupRenderingState(core);

    BoundingFrustum frustum;
    XMFLOAT3 camPos = {};
    if (cam) 
    {
        frustum = cam->GetViewFrustum();
        camPos = cam->GetPosition();
    }

    XMVECTOR camPosVec = XMLoadFloat3(&camPos);

    vector<pair<float, shared_ptr<GameObject>>> alphaObjects;

    for (const auto& obj : objects)
    {
        if (obj->GetId() == -1) continue;
        if (cam && !obj->IsVisible(frustum, camPosVec)) continue;

        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->GetVertexIndexBuffer()) continue;

        bool hasAlpha = false;
        const auto& originalData = mesh->GetOriginalMaterialData();
        for (const auto& mat : originalData) {
            if (!mat.alphaTexPath.empty()) {
                hasAlpha = true;
                break;
            }
        }

        if (hasAlpha) {
            auto transform = obj->GetComponent<Transform>();
            XMFLOAT3 objPos = transform->GetPosition();
            XMVECTOR objPosVec = XMLoadFloat3(&objPos);
            float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(objPosVec, camPosVec)));
            alphaObjects.push_back({ dist, obj });
        }
    }

    sort(alphaObjects.begin(), alphaObjects.end(), [](const auto& a, const auto& b) {
        return a.first > b.first; });

    for (const auto& [dist, obj] : alphaObjects)
    {
        if (cbIndex >= MAX_OBJECTS) {
            OutputDebugStringA("cbIndex Overflowed!!\n");
            break;
        }

        auto mesh = obj->GetComponent<Mesh>();
        auto animator = obj->GetComponent<Animator>();
        auto transform = obj->GetComponent<Transform>();

        if (animator) {
            cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());
        }

        XMMATRIX world = XMMatrixTranspose(transform->GetWorldMatrix());
        mesh->GetVertexIndexBuffer()->Bind(cmdList);

        if (mesh->HasMultiMaterial())
        {
            const auto& subMeshes = mesh->GetSubMeshes();
            const auto& materials = mesh->GetMaterials();
            const auto& originalData = mesh->GetOriginalMaterialData();

            for (size_t i = 0; i < subMeshes.size(); ++i)
            {
                bool hasAlpha = !originalData[i].alphaTexPath.empty();
                if (!hasAlpha) continue;

                auto objConst = MakeObjectConstants(world, 1, 0, materials[i]->GetMaterialIndex());
                size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
                objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
                cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
                cbIndex++;

                mesh->GetVertexIndexBuffer()->DrawIndexed(cmdList, subMeshes[i].indexCount, subMeshes[i].startIndex);
            }
        }
        else
        {
            UINT matIndex = mesh->GetMaterial() ? mesh->GetMaterial()->GetMaterialIndex() : 0;
            auto objConst = MakeObjectConstants(world, 1, 0, matIndex);
            size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
            objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
            cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
            cbIndex++;

            mesh->GetVertexIndexBuffer()->Draw(cmdList);
        }
    }

    if (GetAsyncKeyState('P') & 0x8000)
    {
        string msg = "[Forward Pass] Index: " + to_string(startIndex) + " ~ " + to_string(cbIndex)
            + " (Count: " + to_string(cbIndex - startIndex) + ")\n";
        string totalMsg = ">> Total CB Usage: " + to_string(cbIndex) + " / " + to_string(MAX_OBJECTS) + "\n\n";
        OutputDebugStringA(msg.c_str());
        OutputDebugStringA(totalMsg.c_str());
    }
}

void SceneRenderer::RenderShadow(DX12Core& core, const vector<shared_ptr<GameObject>>& objects)
{
    UINT startIndex = cbIndex;

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Shadow));
    SetupRenderingState(core);

    for (const auto& obj : objects)
    {
        if (obj->GetId() == -1) continue;

        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->GetVertexIndexBuffer()) continue;

        if (cbIndex >= MAX_OBJECTS) {
            OutputDebugStringA("cbIndex Overflowed!!\n");
            break;
        }

        auto animator = obj->GetComponent<Animator>();
        if (animator) {
            cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());
        }

        auto transform = obj->GetComponent<Transform>();
        XMMATRIX world = XMMatrixTranspose(transform->GetWorldMatrix());

        mesh->GetVertexIndexBuffer()->Bind(cmdList);

        if (mesh->HasMultiMaterial())
        {
            const auto& subMeshes = mesh->GetSubMeshes();
            const auto& materials = mesh->GetMaterials();

            for (size_t i = 0; i < subMeshes.size(); ++i)
            {
                auto objConst = MakeObjectConstants(world, 0, 0, materials[i]->GetMaterialIndex());
                size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
                objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
                cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
                cbIndex++;

                mesh->GetVertexIndexBuffer()->DrawIndexed(cmdList, subMeshes[i].indexCount, subMeshes[i].startIndex);
            }
        }
        else
        {
            UINT matIndex = mesh->GetMaterial() ? mesh->GetMaterial()->GetMaterialIndex() : 0;
            auto objConst = MakeObjectConstants(world, 0, 0, matIndex);
            size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
            objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
            cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
            cbIndex++;

            mesh->GetVertexIndexBuffer()->Draw(cmdList);
        }
    }

    if (GetAsyncKeyState('P') & 0x8000)
    {
        string msg = "[Shadow Pass] Index: " + to_string(startIndex) + " ~ " + to_string(cbIndex)
            + " (Count: " + to_string(cbIndex - startIndex) + ")\n";
        OutputDebugStringA(msg.c_str());
    }
}

void SceneRenderer::RenderInstanced(DX12Core& core, Mesh* mesh, UINT instanceCount, UploadBuffer* instanceBuffer, InstancingBatch* batch)
{
    if (!mesh || !mesh->GetVertexIndexBuffer() || !instanceBuffer) return;

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBufferInstancing));
    SetupRenderingState(core, instanceBuffer);

    mesh->GetVertexIndexBuffer()->Bind(cmdList);

    if (mesh->HasMultiMaterial())
    {
        const auto& subMeshes = mesh->GetSubMeshes();

        for (size_t i = 0; i < subMeshes.size(); ++i)
        {
            cmdList->SetGraphicsRootConstantBufferView(1, batch->GetCBAddress(i));
            mesh->GetVertexIndexBuffer()->DrawIndexedInstanced(cmdList, subMeshes[i].indexCount, instanceCount, subMeshes[i].startIndex);
        }
    }
    else
    {
        cmdList->SetGraphicsRootConstantBufferView(1, batch->GetCBAddress(0));
        mesh->GetVertexIndexBuffer()->DrawInstanced(cmdList, instanceCount);
    }
}

void SceneRenderer::RenderInstancedShadow(DX12Core& core, Mesh* mesh, UINT instanceCount, UploadBuffer* instanceBuffer, InstancingBatch* batch)
{
    if (!mesh || !mesh->GetVertexIndexBuffer() || !instanceBuffer) return;

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Shadow));
    SetupRenderingState(core, instanceBuffer);

    mesh->GetVertexIndexBuffer()->Bind(cmdList);

    if (mesh->HasMultiMaterial())
    {
        const auto& subMeshes = mesh->GetSubMeshes();

        for (size_t i = 0; i < subMeshes.size(); ++i)
        {
            cmdList->SetGraphicsRootConstantBufferView(1, batch->GetCBAddress(i));
            mesh->GetVertexIndexBuffer()->DrawIndexedInstanced(cmdList, subMeshes[i].indexCount, instanceCount, subMeshes[i].startIndex);
        }
    }
    else
    {
        cmdList->SetGraphicsRootConstantBufferView(1, batch->GetCBAddress(0));
        mesh->GetVertexIndexBuffer()->DrawInstanced(cmdList, instanceCount);
    }
}

void SceneRenderer::RenderCollisionMeshWireframe(DX12Core& core, const vector<shared_ptr<GameObject>>& objects)
{
    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBufferWireframe));
    SetupRenderingState(core);

    for (const auto& obj : objects)
    {
        if (obj->GetId() == -1) continue;

        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->IsCollisionMeshVisible()) continue;

        if (cbIndex >= MAX_OBJECTS) {
            OutputDebugStringA("cbIndex Overflowed!!\n");
            break;
        }

        auto animator = obj->GetComponent<Animator>();
        if (animator) {
            cmdList->SetGraphicsRootShaderResourceView(10, animator->GetFinalBuffer()->GetGPUVirtualAddress());
        }

        auto transform = obj->GetComponent<Transform>();
        XMMATRIX world = XMMatrixTranspose(transform->GetWorldMatrix());

        auto objConst = MakeObjectConstants(world, 0, 0, 0);
        size_t offset = cbIndex * CONSTANT_BUFFER_ALIGNMENT;
        objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), offset);
        cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress() + offset);
        cbIndex++;

        mesh->GetCollisionMeshBuffer()->Bind(cmdList);
        mesh->GetCollisionMeshBuffer()->Draw(cmdList);
    }
}

void SceneRenderer::RenderTerrain(DX12Core& core, Terrain* terrain)
{
    if (!terrain) return;

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBuffer));
    SetupRenderingState(core);

    cmdList->SetGraphicsRootConstantBufferView(1, terrain->GetCBAddress());

    terrain->Render(cmdList);
}

void SceneRenderer::RenderWater(DX12Core& core, Water* water)
{
    if (!water) return;

    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Opaque));
    SetupRenderingState(core);

    cmdList->SetGraphicsRootConstantBufferView(1, water->GetCBAddress());

    water->Render(core, cmdList);
}

void SceneRenderer::ReleaseUploadBuffer()
{
    if (objectCBPool)
        objectCBPool.reset();

    OutputDebugStringA("SceneRenderer's UploadBuffer has been deleted!!\n");
}

void SceneRenderer::SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer)
{
    auto cmdList = core.GetGraphicsCmdList();

    cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
    Material::BindBindlessResources(cmdList);
    cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

    if (instanceBuffer) {
        cmdList->SetGraphicsRootShaderResourceView(12, instanceBuffer->GetGPUVirtualAddress());
    }
}

ObjectConstants SceneRenderer::MakeObjectConstants(const XMMATRIX& world, int hasTexture, int doInstancing, UINT matIndex)
{
    ObjectConstants obj = {};
    obj.world = world;
    obj.useTexture = hasTexture;
    obj.useInstancing = doInstancing;
    obj.materialIndex = matIndex;
    return obj;
}