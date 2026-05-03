#pragma once
#include "Importer.h"

class Texture;

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
        const MaterialData& matData, const wstring& texBasePath = L"../Assets/FBXModel/");

    static UINT RegisterCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& ddsPath);
    static UINT RegisterTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);
    static UINT RegisterLUT(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);
    static UINT RegisterTextureFromMemory(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, UINT width, UINT height, DXGI_FORMAT format);
    static void RegisterHDRSceneSRV(ID3D12Device* device, ID3D12Resource* hdrSceneRT);
    static void RegisterBloomMipSRV(ID3D12Device* device, ID3D12Resource* bloomTex, UINT mipLevel, UINT slotOffset);

    UINT GetMaterialIndex() const { return materialIndex; }

    static void InitializeBindlessSystem(ID3D12Device* device);
    static void BindBindlessResources(ID3D12GraphicsCommandList* cmdList);
    static void UpdateMaterialBuffer();
    static shared_ptr<Material> FromExistingIndex(UINT idx);
    static void ReleaseUploadBuffers();
    static void Cleanup();

    static constexpr UINT HDR_SCENE_BINDLESS_INDEX = 9000;  // 9000          HDR Scene (단일 슬롯)
    static constexpr UINT BLOOM_CHAIN_LENGTH = 6;           // Bloom mip-chain 길이
    static constexpr UINT BLOOM_MIP_BASE = 9001;            // [9001, 9007)  Bloom mip SRV

private:
    UINT materialIndex = 0xFFFFFFFF;

    static ComPtr<ID3D12DescriptorHeap> bindlessHeap;
    static unique_ptr<UploadBuffer> materialBuffer;
    static vector<MaterialGPUData> materials;
    static vector<unique_ptr<Texture>> allTextures;  

    static UINT nextTextureIndex;
    static UINT nextCubeMapIndex;
    static UINT nextTexture3DIndex;

    static UINT descriptorSize;
    static bool bufferDirty;
    static unordered_map<wstring, UINT> texturePathToIndex;

    static constexpr UINT TEXTURE_2D_BASE = 0;              // [0,    3000)  2D 텍스처
    static constexpr UINT CUBE_MAP_BASE = 3000;             // [3000, 6000)  큐브맵
    static constexpr UINT TEXTURE_3D_BASE = 6000;           // [6000, 9000)  3D 텍스처 (LUT)
    static constexpr UINT BINDLESS_HEAP_SIZE = 10000;
    static constexpr UINT MATERIAL_CAPACITY = 5000;
};
