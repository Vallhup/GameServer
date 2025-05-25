#include "pch.h"
#include "RootSignature.h"

void RootSignature::Initialize(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER rootParams[1];

	rootParams[0].InitAsConstantBufferView(0);	// b0

	CD3DX12_ROOT_SIGNATURE_DESC desc{};
	desc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
