#include "pch.h"
#include "ReflectionSphere.h"
#include "DX12Core.h"
#include "VertexIndexBuffer.h"
#include "Shader.h"
#include "RootSignature.h"
#include "Material.h"

void ReflectionSphere::Initialize(DX12Core& core, int sphereCountX, int sphereCountY)
{
    countX = sphereCountX;
    countY = sphereCountY;

    CreateSphereMesh(core.GetDevice(), core.GetGraphicsCmdList());

    sphereCB = make_unique<UploadBuffer>();
    sphereCB->Initialize(core.GetDevice(), 256 * countX * countY);

    OutputDebugStringA("ReflectionSphere initialized!\n");
}

void ReflectionSphere::CreateSphereMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    vector<Vertex> vertices;
    vector<UINT> indices;

    const int stacks = 32;
    const int slices = 32;

    // 정점 생성
    for (int i = 0; i <= stacks; ++i)
    {
        float phi = XM_PI * i / stacks;
        float y = cos(phi) * radius;
        float r = sin(phi) * radius;

        for (int j = 0; j <= slices; ++j)
        {
            float theta = 2.0f * XM_PI * j / slices;
            float x = r * cos(theta);
            float z = r * sin(theta);

            Vertex v = {};
            v.pos = { x, y, z };
            v.normal = { x / radius, y / radius, z / radius };
            v.uv = { (float)j / slices, (float)i / stacks };
            v.tangent = { -sin(theta), 0, cos(theta) };
            v.color = { 1, 1, 1, 1 };
            v.weights = { 0, 0, 0, 0 };
            v.indices = { 0, 0, 0, 0 };

            vertices.push_back(v);
        }
    }

    // 인덱스 생성
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    sphereMesh = make_shared<VertexIndexBuffer>();
    sphereMesh->Initialize(device, cmdList, vertices, indices);
}

void ReflectionSphere::Render(DX12Core& core)
{
    if (!enabled || !sphereMesh) return;

    auto cmdList = core.GetGraphicsCmdList();

    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::GBuffer));
    cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
    Material::BindBindlessResources(cmdList);
    cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());

    sphereMesh->Bind(cmdList);

    int idx = 0;
    for (int y = 0; y < countY; ++y)
    {
        float metallic = (float)y / max(countY - 1, 1);

        for (int x = 0; x < countX; ++x)
        {
            float roughness = (float)x / max(countX - 1, 1);

            XMFLOAT3 pos = {
                basePosition.x + (x - countX / 2.0f) * spacing,
                basePosition.y + y * spacing,
                basePosition.z
            };

            XMMATRIX world = XMMatrixTranspose(XMMatrixTranslation(pos.x, pos.y, pos.z));

            ObjectConstants constants = {};
            constants.world = world;
            constants.useTexture = 2;  // 특수 플래그: 구체 모드
            constants.useInstancing = 0;
            constants.materialIndex = 0xFFFFFFFF;
            constants.metallic = metallic;
            constants.roughness = roughness;

            size_t offset = idx * 256;
            sphereCB->CopyData(&constants, sizeof(ObjectConstants), offset);
            cmdList->SetGraphicsRootConstantBufferView(1, sphereCB->GetGPUVirtualAddress() + offset);

            sphereMesh->Draw(cmdList);
            idx++;
        }
    }
}