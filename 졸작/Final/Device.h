#pragma once

class Device
{
public:
	void Initialize(HWND hwnd);

	ComPtr<IDXGIFactory6> GetDXGI() const;
	ComPtr<ID3D12Device> GetDevice() const;

private:
	void CreateDXGI(HWND hwnd);
	void CreateDevice();

private:
	ComPtr<IDXGIFactory6> dxgi;
	ComPtr<ID3D12Device> device;
};
