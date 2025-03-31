#pragma once

class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	void Init(uint32 size, uint32 count);

	void Clear();
	void PushData(int32 rootParamIndex, void* buffer, uint32 size);

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32 index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32 index);

private:
	void CreateBuffer();
	void CreateView();

private:
	ComPtr<ID3D12Resource>	_cbvBuffer;
	BYTE*					_mappedBuffer = nullptr;	// CPU쪽에서 데이터를 밀어 넣을때 사용하는 버퍼
	uint32					_elementSize = { 0 };		// Buffer의 크기
	uint32					_elementCount = { 0 };		// Buffer의 개수

	ComPtr<ID3D12DescriptorHeap>	_cbvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		_cpuHandleBegin = {};
	uint32							_handleIncrementSize = { 0 };

	uint32					_currentIndex = { 0 };		// 지금 내가 어디까지 사용햇는지? (한 프레임 완료시 0으로 초기화)
};
