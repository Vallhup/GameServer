#include "pch.h"
#include "SceneRenderer.h"
#include "DX12Core.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Animator.h"
#include "Transform.h"
#include "Material.h"
#include "VertexIndexBuffer.h"
#include "Shader.h"
#include "RootSignature.h"

void SceneRenderer::Initialize(ID3D12Device* device)
{
    objectCBPool = make_unique<UploadBuffer>();
    objectCBPool->Initialize(device, CONSTANT_BUFFER_ALIGNMENT * MAX_OBJECTS);
}

void SceneRenderer::RenderDeferred(DX12Core& core, const vector<shared_ptr<GameObject>>& objects)
{
    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBuffer));
    SetupRenderingState(core);

    size_t cbIndex = 0;
    for (const auto& obj : objects)
    {
        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->GetVertexIndexBuffer()) continue;

        auto animator = obj->GetComponent<Animator>();
        if (animator) {
            animator->ExecuteComputeShader(core);
            cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBuffer));
            SetupRenderingState(core);
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
                if (hasAlpha) continue;  // Forward에서 처리

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
}

void SceneRenderer::RenderForward(DX12Core& core, const vector<shared_ptr<GameObject>>& objects)
{
    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Transparent));
    SetupRenderingState(core);

    size_t cbIndex = 0;
    for (const auto& obj : objects)
    {
        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->GetVertexIndexBuffer()) continue;

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
                if (!hasAlpha) continue;  // Deferred에서 이미 처리됨

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
}

void SceneRenderer::RenderShadow(DX12Core& core, const vector<shared_ptr<GameObject>>& objects)
{
    auto cmdList = core.GetGraphicsCmdList();
    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Shadow));
    SetupRenderingState(core);

    size_t cbIndex = 0;
    for (const auto& obj : objects)
    {
        auto mesh = obj->GetComponent<Mesh>();
        if (!mesh || !mesh->GetVertexIndexBuffer()) continue;

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
}

void SceneRenderer::RenderInstanced(DX12Core& core, Mesh* mesh, UINT instanceCount, UploadBuffer* instanceBuffer)
{
    if (!mesh || !mesh->GetVertexIndexBuffer() || !instanceBuffer) return;

    auto cmdList = core.GetGraphicsCmdList();
    SetupRenderingState(core, instanceBuffer);

    UINT matIndex = mesh->GetMaterial() ? mesh->GetMaterial()->GetMaterialIndex() : 0xFFFFFFFF;
    auto objConst = MakeObjectConstants(XMMatrixIdentity(), mesh->GetMaterial() ? 1 : 0, 1, matIndex);

    objectCBPool->CopyData(&objConst, sizeof(ObjectConstants), 0);
    cmdList->SetGraphicsRootConstantBufferView(1, objectCBPool->GetGPUVirtualAddress());

    mesh->GetVertexIndexBuffer()->Bind(cmdList);
    mesh->GetVertexIndexBuffer()->DrawInstanced(cmdList, instanceCount);
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