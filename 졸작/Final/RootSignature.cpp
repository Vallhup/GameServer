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
    AddCBV(4);              // [4]  b4 - ForwardLight
    AddCBV(5);              // [5]  b5 - ShadowFrameCB

    AddBindlessTable(1);    // [6]  t0, space1 - Bindless
    AddSRV(0, 0);           // [7]  t0 - Material
    AddSRV(1, 0);           // [8]  t1 - Bone Frame
    AddSRV(2, 0);           // [9]  t2 - Offset
    AddSRV(3, 0);           // [10] t3 - Final Bone
    AddUAV(0, 0);           // [11] u0 - Animation R/W
    AddSRV(0, 2);           // [12] t0, space2 - Instance
    AddSRVTable(4, 6, 0);   // [13] t4-t8 - G-Buffer

    AddCBV(6);              // [14] b6 - Fog Constants

    AddBindlessTable(3);    // [15] t0, space3 - Bindless CubeMaps

    AddConstant(1, 7);      // [16] b7 - Cascade shadow index
    AddCBV(8);              // [17] b8 - SsaoCB
    AddSRVTable(0, 4, 4);   // [18] t0-t3, space4 - Ssao SRVs

    AddBindlessTable(5);    // [19] t0, space5 - Bindless 3D Textures For LUT

    AddCBV(9);              // [20] b9 - SkyboxCB

    AddCBV(10);             // [21] b10 - WaterCB

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3];
    samplerDesc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    samplerDesc[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT,         
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    samplerDesc[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    CD3DX12_ROOT_SIGNATURE_DESC desc{};
    desc.Init(static_cast<UINT>(rootParams.size()), rootParams.data(),
        3, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
