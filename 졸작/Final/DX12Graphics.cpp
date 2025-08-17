#include "pch.h"
#include "DX12Graphics.h"
#include "Device.h"
#include "SwapChain.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "Shader.h"
#include "UploadBuffer.h"
#include "DepthStencilView.h"
#include "VertexIndexBuffer.h"

DX12Graphics& DX12Graphics::Get()
{
	static DX12Graphics graphics;
	return graphics;
}

void DX12Graphics::Initialize(HWND hwnd)
{
	device = make_unique<Device>();
	swapChain = make_shared<SwapChain>();
	cmdQueue = make_unique<CommandQueue>();
	rootSig = make_unique<RootSignature>();
	shader = make_unique<Shader>();
	frameCB = make_unique<UploadBuffer>();
	sceneCB = make_unique<UploadBuffer>();
	animationCB = make_unique<UploadBuffer>();
	depthStencilBuffer = make_unique<DepthStencilBuffer>();

	device->Initialize(hwnd);
	cmdQueue->Initialize(device->GetDevice().Get());
	swapChain->Initialize(hwnd, device->GetDXGI().Get(), device->GetDevice().Get(), cmdQueue->GetCmdQueue().Get());
	rootSig->Initialize(device->GetDevice().Get());
	shader->Initialize(device->GetDevice().Get(), rootSig->Get(), L"BasicVS.hlsli", L"BasicPS.hlsli");
	shader->InitializeComputeShader(device->GetDevice().Get(), rootSig->Get(), L"Animation.hlsli");
	frameCB->Initialize(device->GetDevice().Get(), sizeof(XMMATRIX) * 2);
	sceneCB->Initialize(device->GetDevice().Get(), 256 * 100);
	animationCB->Initialize(device->GetDevice().Get(), sizeof(AnimationConstants));
	depthStencilBuffer->Initialize(device->GetDevice().Get());
}

void DX12Graphics::FlushCommandQueue()
{
	cmdQueue->GetCmdList()->Close();

	ID3D12CommandList* lists[] = { cmdQueue->GetCmdList().Get() };
	cmdQueue->GetCmdQueue()->ExecuteCommandLists(1, lists);

	cmdQueue->WaitSync();
}

void DX12Graphics::ResetCommandQueue()
{
	cmdQueue->GetCmdAlloc()->Reset();
	cmdQueue->GetCmdList()->Reset(cmdQueue->GetCmdAlloc().Get(), nullptr);
}

void DX12Graphics::RenderBegin(D3D12_VIEWPORT viewport, D3D12_RECT scissorRect)
{
	cmdQueue->RenderBegin(viewport, scissorRect, swapChain, depthStencilBuffer->GetDSVCpuHandle());
}

void DX12Graphics::RenderEnd()
{
	cmdQueue->RenderEnd(swapChain);
}

UploadBuffer* DX12Graphics::GetFrameCB() const
{
	return frameCB.get();
}

UploadBuffer* DX12Graphics::GetSceneCB() const
{
	return sceneCB.get();
}

UploadBuffer* DX12Graphics::GetAnimationCB() const
{
	return animationCB.get();
}

Device* DX12Graphics::GetDevice() const
{
	return device.get();
}

CommandQueue* DX12Graphics::GetCmdQueue() const
{
	return cmdQueue.get();
}

RootSignature* DX12Graphics::GetRootSig() const
{
	return rootSig.get();
}

Shader* DX12Graphics::GetShader() const
{
	return shader.get();
}

