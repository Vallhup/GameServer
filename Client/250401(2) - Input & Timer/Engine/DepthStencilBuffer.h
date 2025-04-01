#pragma once

class DepthStencilBuffer
{
public:
	void Init(const WindowInfo& window, DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT);	// Stencil 용도 추가위해선
																						// dsvFormat 변경
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() { return _dsvHandle; }
	DXGI_FORMAT GetDSVFormat() { return _dsvFormat; }

private:
	ComPtr<ID3D12Resource>			_dsvBuffer;
	ComPtr<ID3D12DescriptorHeap>	_dsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		_dsvHandle = {};
	DXGI_FORMAT						_dsvFormat = {};
};

