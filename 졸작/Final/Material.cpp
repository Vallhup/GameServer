#include "pch.h"
#include "Material.h"
#include "DX12Graphics.h"
#include "DescriptorHeap.h"
#include "Texture.h"

int Material::nextStartIndex = 2;

void Material::LoadFromMaterialData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const MaterialData& matData, DescriptorHeap* descHeap)
{
    materialData = matData;
    UINT baseSlot = nextStartIndex;

    struct TextureInfo {
        const string& path;
        const string& name;
        UINT slotOffset;
    };

    vector<TextureInfo> textureInfos = {
        {matData.baseColorTexPath, "BaseColor", 0},
        {matData.normalTexPath, "Normal", 1},
        {matData.roughnessTexPath, "Roughness", 2},
        {matData.metallicTexPath, "Metallic", 3},
        {matData.heightTexPath, "Height", 4},
        {matData.alphaTexPath, "Alpha", 5},
        {matData.emissionTexPath, "Emission", 6},
        {matData.aoTexPath, "AO", 7},
    };

    for (const auto& info : textureInfos)
        OutputDebugStringA((info.name + " path: " + info.path + "\n").c_str());

    for (const auto& info : textureInfos)
    {
        if (!info.path.empty()) {
            auto texture = make_unique<Texture>();
            wstring wpath = L"../FBXOutput/" + wstring(info.path.begin(), info.path.end());
            texture->Initialize(device, cmdList, wpath);

            UINT slotIndex = baseSlot + info.slotOffset;
            texture->CreateSRV(device, descHeap, slotIndex);
            textures.push_back(move(texture));
            descriptorIndices.push_back(slotIndex);

            OutputDebugStringA(("  t" + to_string(slotIndex) + " -> " + info.name + "\n").c_str());
        }
    }

    nextStartIndex = baseSlot + 8;
}

void Material::BindToShader(ID3D12GraphicsCommandList* cmdList, UINT rootParamIndex)
{
    if (!textures.empty()) {
        UINT firstIndex = descriptorIndices[0];
        //OutputDebugStringA(("Binding texture at descriptor index: " + to_string(firstIndex) + "\n").c_str());
        D3D12_GPU_DESCRIPTOR_HANDLE handle = GET(DX12Graphics).GetDescHeap()->GetGPUHandle(firstIndex);
        cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, handle);
    }
}

void Material::ResetStartIndex()
{
    nextStartIndex = 2;
}
