#pragma once
#include "Importer.h"

class DescriptorHeap;
class Texture;

class Material
{
public:
    void LoadFromMaterialData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
        const MaterialData& matData, DescriptorHeap* descHeap);

    void BindToShader(ID3D12GraphicsCommandList* cmdList, UINT rootParamIndex);

    const MaterialData& GetMaterialData() const { return materialData; }

    static void ResetStartIndex();

private:
    MaterialData materialData;
    vector<unique_ptr<Texture>> textures;
    vector<UINT> descriptorIndices;  // DescriptorHeap에서의 인덱스들
    static int nextStartIndex;
};
