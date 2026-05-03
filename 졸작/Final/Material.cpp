#include "pch.h"
#include "Material.h"
#include "Texture.h"

ComPtr<ID3D12DescriptorHeap> Material::bindlessHeap = nullptr;
unique_ptr<UploadBuffer> Material::materialBuffer = nullptr;
vector<MaterialGPUData> Material::materials;
vector<unique_ptr<Texture>> Material::allTextures;
UINT Material::nextTextureIndex = 0;
UINT Material::nextCubeMapIndex = 0;
UINT Material::nextTexture3DIndex = 0;
UINT Material::descriptorSize = 0;
bool Material::bufferDirty = false;
unordered_map<wstring, UINT> Material::texturePathToIndex;

void Material::InitializeBindlessSystem(ID3D12Device* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = BINDLESS_HEAP_SIZE;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&bindlessHeap));
    MASSERT(SUCCEEDED(hr), "Failed to create bindless heap");

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    materialBuffer = make_unique<UploadBuffer>();
    materialBuffer->Initialize(device, sizeof(MaterialGPUData) * MATERIAL_CAPACITY);

    OutputDebugStringA("Bindless material system initialized!\n");
}

void Material::LoadFromMaterialData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
    const MaterialData& matData, const wstring& texBasePath)
{
    MaterialGPUData gpuMaterial = {};

    struct TextureInfo {
        UINT* texIndex;
        const string& path;
    };

    vector<TextureInfo> textureInfos = {
        {&gpuMaterial.baseColorTexIndex, matData.baseColorTexPath},
        {&gpuMaterial.normalTexIndex, matData.normalTexPath},
        {&gpuMaterial.roughnessTexIndex, matData.roughnessTexPath},
        {&gpuMaterial.metallicTexIndex, matData.metallicTexPath},
        {&gpuMaterial.heightTexIndex, matData.heightTexPath},
        {&gpuMaterial.alphaTexIndex, matData.alphaTexPath},
        {&gpuMaterial.emissionTexIndex, matData.emissionTexPath},
        {&gpuMaterial.aoTexIndex, matData.aoTexPath}
    };

    for (auto& infos : textureInfos)
    {
        *infos.texIndex = infos.path.empty() ? 0xFFFFFFFF :
            RegisterTexture(device, cmdList, texBasePath + wstring(infos.path.begin(), infos.path.end()));
    }

    materials.push_back(gpuMaterial);
    materialIndex = static_cast<UINT>(materials.size() - 1);
    bufferDirty = true;

    if (materialBuffer) {
        size_t offset = materialIndex * sizeof(MaterialGPUData);
        materialBuffer->CopyData(reinterpret_cast<char*>(materials.data()) + offset, sizeof(MaterialGPUData), offset);
    }

    //UpdateMaterialBuffer();

    OutputDebugStringA(("Material created with index: " + to_string(materialIndex) + "\n").c_str());
}

UINT Material::RegisterCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& ddsPath)
{
    if (!bindlessHeap) {
        OutputDebugStringA("Bindless system not initialized!\n");
        return 0xFFFFFFFF;
    }

    auto it = texturePathToIndex.find(ddsPath);
    if (it != texturePathToIndex.end())
        return it->second;

    auto texture = make_unique<Texture>();
    texture->InitializeCubeMap(device, cmdList, ddsPath);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += (CUBE_MAP_BASE + nextCubeMapIndex) * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texture->GetTexture()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MipLevels = texture->GetTexture()->GetDesc().MipLevels;
    srvDesc.TextureCube.MostDetailedMip = 0;

    device->CreateShaderResourceView(texture->GetTexture(), &srvDesc, cpuHandle);

    UINT index = nextCubeMapIndex++;
    allTextures.push_back(move(texture));
    texturePathToIndex[ddsPath] = index;

    OutputDebugStringA(("CubeMap registered at index: " + to_string(index) + "\n").c_str());
    return index;
}

UINT Material::RegisterTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
    const wstring& path)
{
    if (!bindlessHeap) {
        OutputDebugStringA("Bindless system not initialized!\n");
        return 0xFFFFFFFF;
    }

    auto it = texturePathToIndex.find(path);
    if (it != texturePathToIndex.end())
        return it->second;

    auto texture = make_unique<Texture>();
    filesystem::path p(path);
    p.replace_extension(L".dds");
    wstring ddsPath = p.wstring();
    texture->InitializeDDS(device, cmdList, ddsPath);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += nextTextureIndex * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(texture->GetTexture(), &srvDesc, cpuHandle);

    UINT index = nextTextureIndex++;
    allTextures.push_back(move(texture));

    texturePathToIndex[path] = index;
    OutputDebugStringA(("Texture registered at index: " + to_string(index) + "\n").c_str());
    return index;
}

UINT Material::RegisterTextureFromMemory(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, UINT width, UINT height, DXGI_FORMAT format)
{
    if (!bindlessHeap)
        return 0xFFFFFFFF;

    auto texture = make_unique<Texture>();
    texture->InitializeFromMemory(device, cmdList, data, width, height, format);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += nextTextureIndex * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                  = format;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels     = 1;

    device->CreateShaderResourceView(texture->GetTexture(), &srvDesc, cpuHandle);

    UINT index = nextTextureIndex++;
    allTextures.push_back(move(texture));

    OutputDebugStringA(("Memory texture registered at index: " + to_string(index) + "\n").c_str());
    return index;
}

UINT Material::RegisterLUT(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path)
{
    if (!bindlessHeap) {
        OutputDebugStringA("Bindless system not initialized!\n");
        return 0xFFFFFFFF;
    }

    auto it = texturePathToIndex.find(path);
    if (it != texturePathToIndex.end())
        return it->second;

    auto texture = make_unique<Texture>();
    texture->InitializeLUT(device, cmdList, path);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += (TEXTURE_3D_BASE + nextTexture3DIndex) * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texture->GetTexture()->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture3D.MipLevels = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;

    device->CreateShaderResourceView(texture->GetTexture(), &srvDesc, cpuHandle);

    UINT index = nextTexture3DIndex++;
    allTextures.push_back(move(texture));

    texturePathToIndex[path] = index;
    OutputDebugStringA(("3D LUT registered at index: " + to_string(index) + "\n").c_str());
    return index;
}

void Material::RegisterHDRSceneSRV(ID3D12Device* device, ID3D12Resource* hdrSceneRT)
{
    if (!bindlessHeap) {
        OutputDebugStringA("Bindless system not initialized! Cannot register HDR Scene SRV.\n");
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += HDR_SCENE_BINDLESS_INDEX * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(hdrSceneRT, &srvDesc, cpuHandle);

    OutputDebugStringA(("HDR Scene SRV registered in bindless heap at index " + to_string(HDR_SCENE_BINDLESS_INDEX) + "\n").c_str());
}

void Material::RegisterBloomMipSRV(ID3D12Device* device, ID3D12Resource* bloomTex, UINT mipLevel, UINT slotOffset)
{
    if (!bindlessHeap) {
        OutputDebugStringA("Bindless system not initialized! Cannot register Bloom mip SRV.\n");
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = bindlessHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += (BLOOM_MIP_BASE + slotOffset) * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = mipLevel;

    device->CreateShaderResourceView(bloomTex, &srvDesc, cpuHandle);

    OutputDebugStringA(("Bloom mip " + to_string(mipLevel) + " SRV registered at bindless index " + to_string(BLOOM_MIP_BASE + slotOffset) + "\n").c_str());
}

void Material::BindBindlessResources(ID3D12GraphicsCommandList* cmdList)
{
    if (bindlessHeap && materialBuffer) {
        ID3D12DescriptorHeap* heaps[] = { bindlessHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = bindlessHeap->GetGPUDescriptorHandleForHeapStart();
        cmdList->SetGraphicsRootDescriptorTable(6, gpuHandle);                                      

        D3D12_GPU_DESCRIPTOR_HANDLE cubeMapHandle = gpuHandle;
        cubeMapHandle.ptr += CUBE_MAP_BASE * descriptorSize;
        cmdList->SetGraphicsRootDescriptorTable(15, cubeMapHandle);

        cmdList->SetGraphicsRootShaderResourceView(7, materialBuffer->GetGPUVirtualAddress());

        D3D12_GPU_DESCRIPTOR_HANDLE tex3DHandle = gpuHandle;
        tex3DHandle.ptr += TEXTURE_3D_BASE * descriptorSize;
        cmdList->SetGraphicsRootDescriptorTable(19, tex3DHandle);
    }
}

void Material::UpdateMaterialBuffer()
{
    if (bufferDirty && !materials.empty() && materialBuffer) {
        materialBuffer->CopyData(materials.data(), materials.size() * sizeof(MaterialGPUData));
        bufferDirty = false;
        OutputDebugStringA("Material buffer updated!\n");
    }
}

shared_ptr<Material> Material::FromExistingIndex(UINT idx)
{
    auto m = make_shared<Material>();
    m->materialIndex = idx;
    return m;
}

void Material::ReleaseUploadBuffers()
{
    for (auto& texture : allTextures) {
        if (texture) {
            texture->ReleaseUploadBuffer();
        }
    }
    OutputDebugStringA("Material upload buffers released!\n");
}

void Material::Cleanup()
{
    bindlessHeap.Reset();
    materialBuffer.reset();
    materials.clear();
    allTextures.clear();
    texturePathToIndex.clear();
    nextTextureIndex = 0;
    bufferDirty = false;
    OutputDebugStringA("Bindless material system cleaned up!\n");
}