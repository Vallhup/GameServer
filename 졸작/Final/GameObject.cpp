#include "pch.h"
#include "GameObject.h"
#include "Component.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Transform.h"

void GameObject::Update(float deltaTime)
{
	for (auto& [type, comp] : components) {
		comp->Update(deltaTime);
	}
}

void GameObject::RenderDebugBoundingBox(DX12Core& core, const XMFLOAT4& color)
{
    if (worldBoundingBox.Extents.x == 0) return;

    XMFLOAT3 corners[8];
    worldBoundingBox.GetCorners(corners);

    struct LineVertex {
        XMFLOAT3 pos;
        XMFLOAT2 uv;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT4 weights;
        XMFLOAT4 indices;
        XMFLOAT4 color;
    };

    LineVertex lineTemplate = {
        {0,0,0},        
        {0,0},          
        {0,1,0},        
        {1,0,0},        
        {0,0,0,0},      
        {0,0,0,0},      
        color           
    };

    LineVertex lines[24];
    fill_n(lines, 24, lineTemplate);

    lines[0].pos = corners[0];     lines[8].pos = corners[4];      lines[16].pos = corners[0];
    lines[1].pos = corners[1];     lines[9].pos = corners[5];      lines[17].pos = corners[4];
    lines[2].pos = corners[1];     lines[10].pos = corners[5];     lines[18].pos = corners[1];
    lines[3].pos = corners[2];     lines[11].pos = corners[6];     lines[19].pos = corners[5];
    lines[4].pos = corners[2];     lines[12].pos = corners[6];     lines[20].pos = corners[2];
    lines[5].pos = corners[3];     lines[13].pos = corners[7];     lines[21].pos = corners[6];
    lines[6].pos = corners[3];     lines[14].pos = corners[7];     lines[22].pos = corners[3];
    lines[7].pos = corners[0];     lines[15].pos = corners[4];     lines[23].pos = corners[7];


    if (!debugLineBuffer) {
        CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(lines));

        core.GetDevice()->CreateCommittedResource(
            &uploadHeap,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&debugLineBuffer)
        );
    }

    void* mappedData = nullptr;
    debugLineBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, lines, sizeof(lines));
    debugLineBuffer->Unmap(0, nullptr);

    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = debugLineBuffer->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(LineVertex);
    vbv.SizeInBytes = sizeof(lines);

    auto cmdList = core.GetGraphicsCmdList();

    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::DebugLine)); 
    cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
    cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

    ObjectConstants objConstants = {};
    objConstants.world = XMMatrixIdentity();
    objConstants.useTexture = 0;
    objConstants.useInstancing = 0;
    objConstants.materialIndex = 0;

    core.GetSceneCB()->CopyData(&objConstants, sizeof(ObjectConstants));
    cmdList->SetGraphicsRootConstantBufferView(1, core.GetSceneCB()->GetGPUVirtualAddress());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    cmdList->IASetVertexBuffers(0, 1, &vbv);
    cmdList->DrawInstanced(24, 1, 0, 0);
}

bool GameObject::IsInFrustum(const BoundingFrustum& frustum) const
{
    BoundingBox worldBox = GetWorldBoundingBox();

    // SAFETY FOR: EFFECTS / CAMERA / VIRTUAL OBJECTS
    if (worldBox.Extents.x <= 0.0f)
        return true;

    return frustum.Intersects(worldBox);
}

bool GameObject::IsInRange(const XMVECTOR& camPos) const
{
    if (!needDistanceCull)
        return true;

    auto objPos = GetComponent<Transform>()->GetPosition();
    XMVECTOR objPosVec = XMLoadFloat3(&objPos);

    float dist = XMVectorGetX(XMVector3Length(camPos - objPosVec));

    return dist <= cullDistance;
}

bool GameObject::IsVisible(const BoundingFrustum& frustum, const XMVECTOR& camPos) const
{
    if (!IsInRange(camPos)) return false;   // First check: should distance cull or not
    return IsInFrustum(frustum);            // Second check: is in frustum?
}
