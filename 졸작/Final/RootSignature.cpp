#include "pch.h"
#include "RootSignature.h"

void RootSignature::Initialize(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER rootParams[10];

	rootParams[0].InitAsConstantBufferView(0);			// register(b0) - view & projection Constant BUFF
	rootParams[1].InitAsConstantBufferView(1);			// register(b1) - object Constant BUFF
	rootParams[2].InitAsConstantBufferView(2);			// register(b2) - animationparams Constant BUFF

	CD3DX12_DESCRIPTOR_RANGE bindlessRange;
	bindlessRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 1);					// register(t0, space1)
	rootParams[3].InitAsDescriptorTable(1, &bindlessRange, D3D12_SHADER_VISIBILITY_PIXEL);	// bindless texture ARRAY

	rootParams[4].InitAsShaderResourceView(0);			// register(t0, space0) - material buffer
	rootParams[5].InitAsShaderResourceView(1);			// register(t1, space0) - animation bone frame structured BUFF
	rootParams[6].InitAsShaderResourceView(2);			// register(t2, space0) - animation offset structured BUFF
	rootParams[7].InitAsUnorderedAccessView(0);			// register(u0)	- animation final Read&Write structured BUFF
	rootParams[8].InitAsShaderResourceView(3);			// register(t3, space0) - finalBone Structured BUFF
	rootParams[9].InitAsShaderResourceView(0, 2);		// register(t0, space2) - instance structured BUFF

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[1];
	samplerDesc[0].Init(								// register(s0) - texture Sampler
		0,		
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
