#include "pch.h"
#include "Texture.h"

void Texture::InitializeDDS(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath)
{
    ScratchImage image;
    HRESULT hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, nullptr, image);
    MASSERT(SUCCEEDED(hr), "Failed to load texture file");

    const TexMetadata& meta = image.GetMetadata();

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = static_cast<UINT>(meta.width);
    desc.Height = static_cast<UINT>(meta.height);
    desc.DepthOrArraySize = 1;
    desc.MipLevels = static_cast<UINT16>(meta.mipLevels);
    desc.Format = meta.format;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
    MASSERT(SUCCEEDED(hr), "Failed to create DDS texture");

    vector<D3D12_SUBRESOURCE_DATA> subresources(meta.mipLevels);
    for (size_t mip = 0; mip < meta.mipLevels; ++mip)
    {
        const Image* img = image.GetImage(mip, 0, 0);
        subresources[mip].pData = img->pixels;
        subresources[mip].RowPitch = img->rowPitch;
        subresources[mip].SlicePitch = img->slicePitch;
    }

    UINT64 uploadSize = GetRequiredIntermediateSize(texture.Get(), 0, (UINT)subresources.size());
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
    hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    MASSERT(SUCCEEDED(hr), "Failed to create DDS upload buffer");

    UpdateSubresources(cmdList, texture.Get(), uploadBuffer.Get(), 0, 0,
        (UINT)subresources.size(), subresources.data());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    OutputDebugStringA("DDS based texture loaded!\n");
}

void Texture::InitializeCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& ddsPath)
{
    ScratchImage image;
    HRESULT hr = LoadFromDDSFile(ddsPath.c_str(), DDS_FLAGS_NONE, nullptr, image);
    MASSERT(SUCCEEDED(hr), "Failed to load DDS cubemap");

    const TexMetadata& meta = image.GetMetadata();

    OutputDebugStringA(("CubeMap: " + to_string(meta.width) + "x" + to_string(meta.height) +
        " Mips:" + to_string(meta.mipLevels) + "\n").c_str());

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = meta.width;
    desc.Height = meta.height;
    desc.DepthOrArraySize = 6;
    desc.MipLevels = (UINT16)meta.mipLevels;
    desc.Format = meta.format;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
    MASSERT(SUCCEEDED(hr), "Failed to create cubemap texture");

    vector<D3D12_SUBRESOURCE_DATA> subresources;
    for (size_t face = 0; face < 6; ++face)
    {
        for (size_t mip = 0; mip < meta.mipLevels; ++mip)
        {
            const Image* img = image.GetImage(mip, face, 0);
            D3D12_SUBRESOURCE_DATA data = {};
            data.pData = img->pixels;
            data.RowPitch = img->rowPitch;
            data.SlicePitch = img->slicePitch;
            subresources.push_back(data);
        }
    }

    UINT64 uploadSize = GetRequiredIntermediateSize(texture.Get(), 0, (UINT)subresources.size());
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
    hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    MASSERT(SUCCEEDED(hr), "Failed to create cubemap upload buffer");

    UpdateSubresources(cmdList, texture.Get(), uploadBuffer.Get(), 0, 0,
        (UINT)subresources.size(), subresources.data());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    OutputDebugStringA("CubeMap loaded!\n");
}

void Texture::InitializeLUT(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& filePath)
{
    ScratchImage image;
    HRESULT hr = LoadFromWICFile(filePath.c_str(), WIC_FLAGS_NONE, nullptr, image);
    MASSERT(SUCCEEDED(hr), "Failed to load texture file");

    const Image* img = image.GetImage(0, 0, 0);
    const UINT LUT_SIZE = 32;
    const UINT bytesPerPixel = img->rowPitch / img->width;

    vector<uint8_t> lut3DData(LUT_SIZE * LUT_SIZE * LUT_SIZE * bytesPerPixel);

    for (UINT z = 0; z < LUT_SIZE; ++z) {
        for (UINT y = 0; y < LUT_SIZE; ++y) {
            for (UINT x = 0; x < LUT_SIZE; ++x) {
                UINT srcX = z * LUT_SIZE + x;
                UINT srcY = (LUT_SIZE - 1) - y;

                size_t srcIdx = (srcY * img->rowPitch) + (srcX * bytesPerPixel);
                size_t dstIdx = ((z * LUT_SIZE * LUT_SIZE) + (y * LUT_SIZE) + x) * bytesPerPixel;

                memcpy(&lut3DData[dstIdx], &img->pixels[srcIdx], bytesPerPixel);
            }
        }
    }

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    textureDesc.Width = LUT_SIZE;
    textureDesc.Height = LUT_SIZE;
    textureDesc.DepthOrArraySize = LUT_SIZE;
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

    UINT64 uploadSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
    hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    MASSERT(SUCCEEDED(hr), "Failed to create DDS upload buffer");

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = lut3DData.data();
    textureData.RowPitch = LUT_SIZE * bytesPerPixel;
    textureData.SlicePitch = LUT_SIZE * LUT_SIZE * bytesPerPixel;

    UpdateSubresources(cmdList, texture.Get(), uploadBuffer.Get(), 0, 0, 1, &textureData);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    OutputDebugStringA("LUT based texture loaded!\n");
}

void Texture::InitializeFromMemory(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, UINT width, UINT height, DXGI_FORMAT format)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width            = width;
    desc.Height           = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.Format           = format;
    desc.SampleDesc.Count = 1;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags            = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    HRESULT hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
    MASSERT(SUCCEEDED(hr), "Failed to create texture from memory");

    UINT64 uploadSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
    hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
        &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    MASSERT(SUCCEEDED(hr), "Failed to create upload buffer for memory texture");

    D3D12_SUBRESOURCE_DATA texData = {};
    texData.pData      = data;
    texData.RowPitch   = width * 4; // R8G8B8A8
    texData.SlicePitch = texData.RowPitch * height;

    UpdateSubresources(cmdList, texture.Get(), uploadBuffer.Get(), 0, 0, 1, &texData);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}
