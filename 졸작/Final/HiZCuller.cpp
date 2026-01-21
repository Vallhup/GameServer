#include "pch.h"
#include "HiZCuller.h"
#include "DX12Core.h"

void HiZCuller::Initialize(ID3D12Device* device, UINT w, UINT h, ID3D12Resource* depthBuffer)
{
    width = w;
    height = h;

	UINT tempW = width, tempH = height;
    mipLevels = 1;
    while (tempW > 1 || tempH > 1) {
        tempW = max(1u, tempW / 2);
        tempH = max(1u, tempH / 2);
        mipLevels++;
    }

    D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, width, height, 1,
        mipLevels, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    device->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&hiZTexture));

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = mipLevels * 2 + 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&hiZHeap));

    descSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = hiZHeap->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < mipLevels; ++i)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = i;
        device->CreateShaderResourceView(hiZTexture.Get(), &srvDesc, cpuHandle);
        cpuHandle.ptr += descSize;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = i;
        device->CreateUnorderedAccessView(hiZTexture.Get(), nullptr, &uavDesc, cpuHandle);
        cpuHandle.ptr += descSize;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(depthBuffer, &depthSrvDesc, cpuHandle);

    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE uavRange;
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParams[2];
    rootParams[0].InitAsDescriptorTable(1, &srvRange);
    rootParams[1].InitAsDescriptorTable(1, &uavRange);

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0, D3D12_FILTER_MIN_MAG_MIP_POINT);
    
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.Init(2, rootParams, 1, &samplerDesc);

    ComPtr<ID3DBlob> serializedRootSig = nullptr, errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        MASSERT(false, "D3D12SerializeRootSignature failed");
    }

    hr = device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    MASSERT(SUCCEEDED(hr), "Failed to create Root Signature");

    ComPtr<ID3DBlob> csBlob, csErrorBlob;
    hr = D3DCompileFromFile(
        L"../Shaders/HiZGenerateCS.hlsli",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "cs_5_1",
        0, 0,
        &csBlob,
        &csErrorBlob);

    if (FAILED(hr)) {
        if (csErrorBlob)
            OutputDebugStringA((char*)csErrorBlob->GetBufferPointer());
        MASSERT(false, "HiZGenerateCS compile failed");
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

    hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&generatePSO));
    MASSERT(SUCCEEDED(hr), "Failed to create HiZ Compute PSO");
}

void HiZCuller::GenerateHiZ(DX12Core& core)
{
    auto cmdList = core.GetGraphicsCmdList();

    // Depth Buffer 상태 전환
    CD3DX12_RESOURCE_BARRIER depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        core.GetDepthBuffer(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &depthBarrier);

    // Heap 바인딩
    ID3D12DescriptorHeap* heaps[] = { hiZHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(rootSignature.Get());
    cmdList->SetPipelineState(generatePSO.Get());

    // Depth SRV 인덱스 (맨 마지막)
    UINT depthSrvIndex = mipLevels * 2;

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(hiZHeap->GetGPUDescriptorHandleForHeapStart());

    // 각 밉 레벨 생성
    for (UINT i = 0; i < mipLevels; ++i)
    {
        UINT srcIndex, dstIndex;

        if (i == 0) {
            // 첫 번째: Depth Buffer → Mip 0
            srcIndex = depthSrvIndex;  // Depth SRV
            dstIndex = 1;              // Mip 0 UAV
        }
        else {
            // 나머지: Mip (i-1) → Mip i
            srcIndex = (i - 1) * 2;    // 이전 밉 SRV
            dstIndex = i * 2 + 1;      // 현재 밉 UAV
        }

        // SRV, UAV 바인딩
        CD3DX12_GPU_DESCRIPTOR_HANDLE srcHandle(gpuHandle, srcIndex, descSize);
        CD3DX12_GPU_DESCRIPTOR_HANDLE dstHandle(gpuHandle, dstIndex, descSize);

        cmdList->SetComputeRootDescriptorTable(0, srcHandle);
        cmdList->SetComputeRootDescriptorTable(1, dstHandle);

        // Dispatch 크기 계산
        UINT mipWidth = max(1u, width >> i);
        UINT mipHeight = max(1u, height >> i);
        UINT dispatchX = (mipWidth + 7) / 8;
        UINT dispatchY = (mipHeight + 7) / 8;

        cmdList->Dispatch(dispatchX, dispatchY, 1);

        // UAV 배리어 (다음 패스에서 읽기 전에)
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(hiZTexture.Get());
        cmdList->ResourceBarrier(1, &barrier);
    }

    // Depth Buffer 상태 복원
    depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        core.GetDepthBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    cmdList->ResourceBarrier(1, &depthBarrier);
}
