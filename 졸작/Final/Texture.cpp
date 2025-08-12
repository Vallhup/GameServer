#include "pch.h"
#include "Texture.h"
#include "DescriptorHeap.h"

void Texture::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath)
{
    ScratchImage image;
    HRESULT hr = LoadFromWICFile(filePath.c_str(), WIC_FLAGS_NONE, nullptr, image);
    MASSERT(SUCCEEDED(hr), "Failed to load texture file");

    const Image* img = image.GetImage(0, 0, 0);

    // 디버깅 로그 추가
    OutputDebugStringA(("Texture size: " + to_string(img->width) + "x" + to_string(img->height) + "\n").c_str());
    OutputDebugStringA(("Texture memory: " + to_string(img->slicePitch) + " bytes\n").c_str());

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = static_cast<UINT>(img->width);
    textureDesc.Height = static_cast<UINT>(img->height);
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = img->format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture));
    MASSERT(SUCCEEDED(hr), "Failed to create texture resource");

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
    OutputDebugStringA(("Upload buffer size: " + to_string(uploadBufferSize) + " bytes\n").c_str());

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer));
    MASSERT(SUCCEEDED(hr), "Failed to create upload buffer");

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = img->pixels;
    textureData.RowPitch = img->rowPitch;
    textureData.SlicePitch = img->slicePitch;

    UpdateSubresources(cmdList, texture.Get(), uploadBuffer.Get(), 0, 0, 1, &textureData);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}

void Texture::InitializeFromRAW(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath, UINT width, UINT height)
{
    std::ifstream file(filePath, std::ios::binary);
    MASSERT(file.is_open(), "Failed to open RAW file");

    std::vector<UINT8> rawData(width * height);
    file.read(reinterpret_cast<char*>(rawData.data()), width * height);
    file.close();

    std::vector<float> heightData(width * height);
    for (size_t i = 0; i < rawData.size(); ++i)
        heightData[i] = rawData[i] / 255.0f;

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
    MASSERT(SUCCEEDED(hr), "Failed to create heightmap texture");

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = heightData.data();
    textureData.RowPitch = width * sizeof(float);
    textureData.SlicePitch = textureData.RowPitch * height;

    UpdateSubresources(cmdList, texture.Get(), uploadBuffer.Get(), 0, 0, 1, &textureData);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}

void Texture::CreateSRV(ID3D12Device* device, DescriptorHeap* descHeap, UINT index)
{
    descHeap->CreateSRV(device, texture.Get(), index);
    srvGpuHandle = descHeap->GetGPUHandle(index);
}