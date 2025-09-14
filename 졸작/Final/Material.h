#pragma once
#include "Importer.h"

class Texture;
class UploadBuffer;

struct MaterialGPUData
{
    UINT baseColorTexIndex;
    UINT normalTexIndex;
    UINT roughnessTexIndex;
    UINT metallicTexIndex;
    UINT heightTexIndex;
    UINT alphaTexIndex;
    UINT emissionTexIndex;
    UINT aoTexIndex;
};

class Material
{
public:
    void LoadFromMaterialData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
        const MaterialData& matData);
    UINT GetMaterialIndex() const { return materialIndex; }

    static void InitializeBindlessSystem(ID3D12Device* device);
    static void BindBindlessResources(ID3D12GraphicsCommandList* cmdList);
    static void UpdateMaterialBuffer();
    static shared_ptr<Material> FromExistingIndex(UINT idx);
    static void ReleaseUploadBuffers();
    static void Cleanup();

private:
    UINT materialIndex = 0xFFFFFFFF;

    static ComPtr<ID3D12DescriptorHeap> bindlessHeap;
    static unique_ptr<UploadBuffer> materialBuffer;
    static vector<MaterialGPUData> materials;
    static vector<unique_ptr<Texture>> allTextures;  
    static UINT nextTextureIndex;
    static UINT descriptorSize;
    static bool bufferDirty;
    static unordered_map<wstring, UINT> texturePathToIndex;

    UINT RegisterTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
        const wstring& path);
};
