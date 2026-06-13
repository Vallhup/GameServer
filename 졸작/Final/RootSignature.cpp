#include "pch.h"
#include "RootSignature.h"

void RootSignature::Initialize(ID3D12Device* device)
{
    std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
    std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>> tables;

    auto AddCBV = [&](UINT reg, UINT space = 0) {
        CD3DX12_ROOT_PARAMETER param;
        param.InitAsConstantBufferView(reg, space);
        rootParams.push_back(param);
        };

    auto AddSRV = [&](UINT reg, UINT space = 0) {
        CD3DX12_ROOT_PARAMETER param;
        param.InitAsShaderResourceView(reg, space);
        rootParams.push_back(param);
        };

    auto AddUAV = [&](UINT reg, UINT space = 0) {
        CD3DX12_ROOT_PARAMETER param;
        param.InitAsUnorderedAccessView(reg, space);
        rootParams.push_back(param);
        };

    auto AddBindlessTable = [&](UINT space = 1) {
        tables.emplace_back(1); 
        auto& r = tables.back()[0];
        r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, space);

        CD3DX12_ROOT_PARAMETER p;
        p.InitAsDescriptorTable(1, tables.back().data(), D3D12_SHADER_VISIBILITY_PIXEL);
        rootParams.push_back(p);
        };

    auto AddSRVTable = [&](UINT startReg, UINT count, UINT space = 0) {
        tables.emplace_back(1);
        auto& r = tables.back()[0];
        r.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, count, startReg, space);

        CD3DX12_ROOT_PARAMETER p;
        p.InitAsDescriptorTable(1, tables.back().data(), D3D12_SHADER_VISIBILITY_PIXEL);
        rootParams.push_back(p);
        };

    auto AddConstant = [&](UINT value, UINT reg) {
        CD3DX12_ROOT_PARAMETER param;
        param.InitAsConstants(value, reg);
        rootParams.push_back(param);
        };

    AddCBV(0);              // [0]  b0 - FrameCB
    AddCBV(1);              // [1]  b1 - ObjectCB
    AddCBV(2);              // [2]  b2 - AnimationParams
    AddCBV(3);              // [3]  b3 - DeferredLight
    AddSRVTable(14, 1, 0);  // [4]  t14 - Point shadow cube array (구 b4 reserved 슬롯 재활용, deferredSRVHeap 슬롯 7)
    AddCBV(5);              // [5]  b5 - ShadowFrameCB

    AddBindlessTable(1);    // [6]  t0, space1 - Bindless
    AddSRV(0, 0);           // [7]  t0 - Material
    AddSRV(1, 0);           // [8]  t1 - Bone Frame
    AddSRV(2, 0);           // [9]  t2 - Offset
    AddSRV(3, 0);           // [10] t3 - Final Bone
    AddUAV(0, 0);           // [11] u0 - Animation R/W
    AddSRV(0, 2);           // [12] t0, space2 - Instance
    AddSRVTable(4, 7, 0);   // [13] t4-t10 - G-Buffer

    AddCBV(6);              // [14] b6 - Fog Constants

    AddBindlessTable(3);    // [15] t0, space3 - Bindless CubeMaps

    AddConstant(1, 7);      // [16] b7 - Cascade shadow index
    AddCBV(8);              // [17] b8 - SsaoCB
    AddSRVTable(0, 4, 4);   // [18] t0-t3, space4 - Ssao SRVs

    AddBindlessTable(5);    // [19] t0, space5 - Bindless 3D Textures For LUT

    AddCBV(9);              // [20] b9 - SkyboxCB
    AddCBV(10);             // [21] b10 - WaterCB
    AddCBV(11);             // [22] b11 - VolumetricFogCB
    AddCBV(12);             // [23] b12 - TrailCB
    AddConstant(8, 13);     // [24] b13 - BloomConstants (8x32bit)
    AddSRV(11, 0);          // [25] t11 - DeferredLight Array (SRV, 1단계 SRV 전환)

    // Clustered Shading
    AddCBV(14);             // [26] b14 - ClusterParamsCB
    AddSRV(12, 0);          // [27] t12 - clusterLightIndices SRV (PS)
    AddSRV(13, 0);          // [28] t13 - clusterLightGrid SRV (PS)
    AddUAV(1, 0);           // [29] u1  - clusterLightIndicesRW UAV (CS)
    AddUAV(2, 0);           // [30] u2  - clusterLightGridRW UAV (CS)
    AddUAV(3, 0);           // [31] u3  - clusterCounterRW UAV (CS atomic)

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc[4];
    samplerDesc[0].Init(0, D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f, 16);   // MaxAnisotropy 16 (비등방성 ,이방성 필터링)

    samplerDesc[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    samplerDesc[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    samplerDesc[3].Init(3, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        0.0f, 16,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

    CD3DX12_ROOT_SIGNATURE_DESC desc{};
    desc.Init(static_cast<UINT>(rootParams.size()), rootParams.data(),
        4, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        MASSERT(false, "D3D12SerializeRootSignature failed");
    }

    hr = device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
    MASSERT(SUCCEEDED(hr), "Failed to create Root Signature");
}

ID3D12RootSignature* RootSignature::Get() const
{
	return rootSignature.Get();
}
