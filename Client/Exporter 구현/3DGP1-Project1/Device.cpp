#include "pch.h"
#include "Device.h"

void Device::Initialize(HWND hwnd)
{
	CreateDXGI(hwnd);
	CreateDevice();
}

ComPtr<IDXGIFactory6> Device::GetDXGI() const
{
	return dxgi;
}

ComPtr<ID3D12Device> Device::GetDevice() const
{
	return device;
}

void Device::CreateDXGI(HWND hwnd)
{
	ASSERT(hwnd != nullptr);

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		RELEASE_COM(debugController);
	}
#endif

	HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgi));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create DXGI factory\n");
}

void Device::CreateDevice()
{
	HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));

	if (FAILED(hr))
		OutputDebugStringA("Failed to create D3D12 device\n");
}
