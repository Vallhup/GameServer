#include "pch.h"
#include "SSAO.h"

void SSAO::Initialize(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = WinSize.x;
	desc.Height = WinSize.y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = desc.Format;
	clearValue.Color[0] = 1.0f;

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&ssaoTexture));
	MASSERT(SUCCEEDED(hr), "Failed to create SSAO Texture");

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&ssaoRTVHeap));
	MASSERT(SUCCEEDED(hr), "Failed to create SSAO RTV Heap");

	ssaoRTVHandle = ssaoRTVHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateRenderTargetView(ssaoTexture.Get(), nullptr, ssaoRTVHandle);

	OutputDebugStringA("SSAO Texture & Descriptors created successfully!!\n");
}

bool SSAO::GetSSAOState() const
{
	return enableSSAO;
}

ID3D12Resource* SSAO::GetSSAOTexture() const
{
	return ssaoTexture.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE SSAO::GetRTVHandle() const
{
	return ssaoRTVHandle;
}

void SSAO::SetSSAOState(bool in)
{
	enableSSAO = in;
}
