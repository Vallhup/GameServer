#pragma once
#include "GameObject.h"

class DX12Core;
class VertexIndexBuffer;

class ReflectionSphere
{
public:
    void Initialize(DX12Core& core, int sphereCountX = 5, int sphereCountY = 2);
    void Render(DX12Core& core);
    void SetPosition(const XMFLOAT3& basePos) { basePosition = basePos; }
    void SetEnabled(bool enable) { enabled = enable; }
    bool IsEnabled() const { return enabled; }

private:
    void CreateSphereMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

private:
    shared_ptr<VertexIndexBuffer> sphereMesh;
    unique_ptr<UploadBuffer> sphereCB;

    XMFLOAT3 basePosition = { 0, 2, 0 };
    int countX = 5;  // roughness 단계
    int countY = 2;  // metallic 단계 (0, 1)
    float spacing = 1.5f;
    float radius = 0.5f;

    bool enabled = false;
};