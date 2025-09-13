#include "pch.h"
#include "Material.h"
#include "Texture.h"
#include "UploadBuffer.h"

ComPtr<ID3D12DescriptorHeap> Material::bindlessHeap = nullptr;
unique_ptr<UploadBuffer> Material::materialBuffer = nullptr;
vector<MaterialGPUData> Material::materials;
vector<unique_ptr<Texture>> Material::allTextures;
UINT Material::nextTextureIndex = 0;  
UINT Material::descriptorSize = 0;
bool Material::bufferDirty = false;

void Material::InitializeBindlessSystem(ID3D12Device* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 1000;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&bindlessHeap));
    MASSERT(SUCCEEDED(hr), "Failed to create bindless heap");

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    materialBuffer = make_unique<UploadBuffer>();
    materialBuffer->Initialize(device, sizeof(MaterialGPUData) * 500);

    OutputDebugStringA("Bindless material system initialized!\n");
}

void Material::LoadFromMaterialData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
    const MaterialData& matData)
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
            RegisterTexture(device, cmdList, L"../FBXOutput/" + wstring(infos.path.begin(), infos.path.end()));
    }

    materials.push_back(gpuMaterial);
    materialIndex = static_cast<UINT>(materials.size() - 1);
    bufferDirty = true;

    UpdateMaterialBuffer();

    OutputDebugStringA(("Material created with index: " + to_string(materialIndex) + "\n").c_str());
}

UINT Material::RegisterTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
    const wstring& path)
{
    if (!bindlessHeap) {
        OutputDebugStringA("Bindless system not initialized!\n");
        return 0xFFFFFFFF;
    }

    auto texture = make_unique<Texture>();
    texture->Initialize(device, cmdList, path);

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

    OutputDebugStringA(("Texture registered at index: " + to_string(index) + "\n").c_str());
    return index;
}

void Material::BindBindlessResources(ID3D12GraphicsCommandList* cmdList)
{
    if (bindlessHeap && materialBuffer) {
        ID3D12DescriptorHeap* heaps[] = { bindlessHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = bindlessHeap->GetGPUDescriptorHandleForHeapStart();
        cmdList->SetGraphicsRootDescriptorTable(5, gpuHandle);                                      // 레지 넘버링 부분

        cmdList->SetGraphicsRootShaderResourceView(6, materialBuffer->GetGPUVirtualAddress());      // 레지 넘버링 부분
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
    nextTextureIndex = 0;
    bufferDirty = false;
    OutputDebugStringA("Bindless material system cleaned up!\n");
}