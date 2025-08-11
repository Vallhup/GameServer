#include "pch.h"
#include "Material.h"
#include "DX12Graphics.h"
#include "DescriptorHeap.h"
#include "Texture.h"

int Material::nextStartIndex = 2;

void Material::LoadFromMaterialData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const MaterialData& matData, DescriptorHeap* descHeap)
{
    OutputDebugStringA(("BaseColor path: " + matData.baseColorTexPath + "\n").c_str());
    OutputDebugStringA(("Normal path: " + matData.normalTexPath + "\n").c_str());
    OutputDebugStringA(("Roughness path: " + matData.roughnessTexPath + "\n").c_str());
    OutputDebugStringA(("Metallic path: " + matData.metallicTexPath + "\n").c_str());
    OutputDebugStringA(("HeightTex path: " + matData.heightTexPath + "\n").c_str());
    OutputDebugStringA(("AlphaTex path: " + matData.alphaTexPath + "\n").c_str());
    OutputDebugStringA(("EmissionTex path: " + matData.emissionTexPath + "\n").c_str());
    OutputDebugStringA(("AO path: " + matData.aoTexPath + "\n").c_str());

    materialData = matData;

    // 텍스처 경로 배열
    vector<string> texPaths = {
        matData.baseColorTexPath,
        matData.normalTexPath,
        matData.roughnessTexPath,
        matData.metallicTexPath,
        matData.heightTexPath,
        matData.alphaTexPath,
        matData.emissionTexPath,
        matData.aoTexPath
    };

    UINT currentIndex = nextStartIndex;  // 0,1은 heightmap, ground용

    for (const auto& path : texPaths) {
        if (!path.empty()) {
            auto texture = make_unique<Texture>();
            wstring wpath = L"../FBXOutput/" + wstring(path.begin(), path.end());
            texture->Initialize(device, cmdList, wpath);
            texture->CreateSRV(device, descHeap, currentIndex);

            textures.push_back(move(texture));
            descriptorIndices.push_back(currentIndex++);
        }
    }

    nextStartIndex = currentIndex;
}

void Material::BindToShader(ID3D12GraphicsCommandList* cmdList, UINT startSlot)
{
    if (!textures.empty()) {
        UINT firstIndex = descriptorIndices[0];
        //OutputDebugStringA(("Binding texture at descriptor index: " + to_string(firstIndex) + "\n").c_str());
        D3D12_GPU_DESCRIPTOR_HANDLE handle = GET(DX12Graphics).GetDescHeap()->GetGPUHandle(firstIndex);
        cmdList->SetGraphicsRootDescriptorTable(4, handle);
    }
}

void Material::ResetStartIndex()
{
    nextStartIndex = 2;
}
