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

    AddCBV(0);              // rootParmas[0] register(b0) - view & projection Constant BUFF
    AddCBV(1);              // rootParmas[1] register(b1) - object Constant BUFF
    AddCBV(2);              // rootParmas[2] register(b2) - animationparams Constant BUFF
    AddCBV(3);              // rootParmas[3] register(b3) - deferred light Constant BUFF
    AddCBV(4);              // rootParmas[4] register(b4) - forward light Constant BUFF
    AddBindlessTable(1);    // rootParmas[5] register(t0, space1) - bindless texture ARRAY
    AddSRV(0, 0);           // rootParmas[6] register(t0, space0) - material buffer
    AddSRV(1, 0);           // rootParmas[7] register(t1, space0) - animation bone frame structured BUFF
    AddSRV(2, 0);           // rootParmas[8] register(t2, space0) - animation offset structured BUFF
    AddSRV(3, 0);           // rootParmas[9] register(t3, space0) - finalBone Structured BUFF
    AddUAV(0, 0);           // rootParmas[10] register(u0)	- animation final Read&Write structured BUFF
    AddSRV(0, 2);           // rootParmas[11] register(t0, space2) - instance structured BUFF
    AddSRVTable(4, 4, 0);   // rootParmas[12] register(t4-t7, space0) - G-Buffer SRV Å×ÀÌºí

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc[1];
    samplerDesc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,         // register(s0) - texture Sampler
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    CD3DX12_ROOT_SIGNATURE_DESC desc{};
    desc.Init(static_cast<UINT>(rootParams.size()), rootParams.data(),
        1, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
