#include "pch.h"
#include "RootSignature.h"

void RootSignature::Initialize(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER rootParams[9];

	rootParams[0].InitAsConstantBufferView(0);	// register(b0) - view & projection Constant BUFF
	rootParams[1].InitAsConstantBufferView(1);	// register(b1) - object Constant BUFF
	rootParams[2].InitAsConstantBufferView(2);	// register(b2) - animationparams Constant BUFF

	CD3DX12_DESCRIPTOR_RANGE materialRange;
	materialRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0);		// register(t0~t7) - material textures
	rootParams[3].InitAsDescriptorTable(1, &materialRange, D3D12_SHADER_VISIBILITY_PIXEL);

	rootParams[4].InitAsShaderResourceView(8);			// register(t8) - animation bone frame structured BUFF
	rootParams[5].InitAsShaderResourceView(9);			// register(t9) - animation offset structured BUFF
	rootParams[6].InitAsUnorderedAccessView(0);			// register(u0) - animation final Read&Write structured BUFF
	rootParams[7].InitAsShaderResourceView(0, 1);		// register(t0) & space1 - instance structured BUFF
	rootParams[8].InitAsShaderResourceView(10);			// register(t10) - finalBone Structured BUFF

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[1];
	samplerDesc[0].Init(
		0,		// register(s0) - texture Sampler
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	desc.Init(_countof(rootParams), rootParams, 1, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig,
		&errorBlob
	);

	if (FAILED(hr))
	{
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());

		MASSERT(false, "D3D12SerializeRootSignature failed");
	}

	hr = device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);

	MASSERT(SUCCEEDED(hr), "Failed to create Root Signature");
}

ID3D12RootSignature* RootSignature::Get() const
{
	return rootSignature.Get();
}
