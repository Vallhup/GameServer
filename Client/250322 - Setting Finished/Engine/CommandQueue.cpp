#include "pch.h"
#include "CommandQueue.h"
#include "SwapChain.h"

CommandQueue::~CommandQueue()
{
	CloseHandle(_fenceEvent);
}

void CommandQueue::Init(ComPtr<ID3D12Device> device, shared_ptr<class SwapChain> swapChain)
{
	_swapChain = swapChain;

	D3D12_COMMAND_QUEUE_DESC queueDesc =
	{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));

	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();

	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CommandQueue::RenderBegin(const D3D12_VIEWPORT* vp, const D3D12_RECT* rect)
{
	// 렌더링 시작과 동시에 항상 명령 할당자와 리스트는 초기화
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_swapChain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	_cmdList->ResourceBarrier(1, &barrier);

	// 명령 리스트에 명령 밀어넣었으면 뷰포트와 가위 재설정 해야함
	_cmdList->RSSetViewports(1, vp);
	_cmdList->RSSetScissorRects(1, rect);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = _swapChain->GetBackRTV();		// 백 버퍼 꺼내 와서
	_cmdList->ClearRenderTargetView(backBufferView, Colors::SkyBlue, 0, nullptr);		// GPU한테 백 버퍼 알려주고
	_cmdList->OMSetRenderTargets(1, &backBufferView, FALSE, nullptr);					// 일감을 그려달라고 요청
}

void CommandQueue::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_swapChain->GetBackRTVBuffer().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	_cmdList->ResourceBarrier(1, &barrier);
	_cmdList->Close();

	ID3D12CommandList* cmdListArr[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	_swapChain->Present();		// 후면 버퍼와 전면 버퍼 교체 (실제로 화면에 그리기)

	WaitSync();					// GPU가 명령 다 처리 할때까지 CPU 대기

	_swapChain->SwapIndex();	// 후면 버퍼 인덱스 바꿔치기
}

void CommandQueue::WaitSync()
{
	_fenceValue++;

	_cmdQueue->Signal(_fence.Get(), _fenceValue);

	if (_fence->GetCompletedValue() < _fenceValue)				// 왜 < 를 쓰냐, _fenceValue 자료형이
	{															// uint64로, 범위가 조건을 커버하여
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);	// 엣지 케이스에 대한 예외 처리가 필요 없기 때문

		WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

// RenderBegin ~ RenderEnd 동작원리
// [RenderBegin]
// 명령 할당자 & 리스트 초기화 (매 프레임마다 필수임 - 안 그러면 할당자는 메모리 문제, 리스트는 중복 렌더링 할거임)
// RenderBegin에서 Barrier
// 후면 버퍼의 상태를 그리는 대상으로 변경
// 뷰포트와 가위 설정
// 일감을 그려달라고 요청
// 
// [RenderEnd]
// RenderEnd에서 Barrier
// 후면 버퍼의 상태를 화면에 표시로 변경
// 리스트 닫기 (닫고 나서 보내는게 원칙임)
// 명령 큐에 명령 리스트 보내기
// 후면 버퍼와 전면버퍼 교체 (실제 화면에 그려지는 부분)
// GPU가 명령을 다 처리하기 전까지 기다리셈(WaitSync)
// 후면버퍼 인덱스 바꿔치기