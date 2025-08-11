#include "pch.h"
#include "RootSignature.h"

void RootSignature::Initialize(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER rootParams[6];

	rootParams[0].InitAsConstantBufferView(0);	// register(b0)
	rootParams[1].InitAsConstantBufferView(1);	// register(b1)

	CD3DX12_DESCRIPTOR_RANGE heightRange;
	heightRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);		// register(t0)
	rootParams[2].InitAsDescriptorTable(1, &heightRange, D3D12_SHADER_VISIBILITY_VERTEX);		// VertexShader에서만 사용

	CD3DX12_DESCRIPTOR_RANGE terrainRange;
	terrainRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);		// register(t1)
	rootParams[3].InitAsDescriptorTable(1, &terrainRange, D3D12_SHADER_VISIBILITY_PIXEL);		// PixelShader에서만 사용

	CD3DX12_DESCRIPTOR_RANGE materialRange;
	materialRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 2);		// t2~t9까지 8개
	rootParams[4].InitAsDescriptorTable(1, &materialRange, D3D12_SHADER_VISIBILITY_PIXEL);

	rootParams[5].InitAsShaderResourceView(0, 1);		// register(t0) & space1

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3];
	
	samplerDesc[0].Init(
		0,		// register(s0)
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	samplerDesc[1].Init(
		1,		// register(s1)
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	samplerDesc[2].Init(
		2,		// register(s2)
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	desc.Init(_countof(rootParams), rootParams, 3, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		IID_PPV_ARGS(&rootsignature)
	);

	MASSERT(SUCCEEDED(hr), "Failed to create Root Signature");
}

ID3D12RootSignature* RootSignature::Get() const
{
	return rootsignature.Get();
}
