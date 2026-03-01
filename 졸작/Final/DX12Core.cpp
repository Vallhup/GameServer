#include "pch.h"
#include "DX12Core.h"
#include "Engine.h"
#include "SceneManager.h"
#include "RootSignature.h"
#include "Shader.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "ShadowMappingManager.h"
#include "RenderTargets.h"
#include "LightManager.h"
#include "FroxelManager.h"

void DX12Core::Initialize(HWND hwnd)
{
	deviceCtx = make_unique<DeviceContext>();
	deviceCtx->Initialize(hwnd);

	swapChainMgr = make_unique<SwapChain>();
	swapChainMgr->Initialize(deviceCtx->GetDxgi(), deviceCtx->GetCmdQueue(), deviceCtx->GetDevice(), hwnd);

	rootSig = make_unique<RootSignature>();
	shader = make_unique<Shader>();
	frameCB = make_unique<UploadBuffer>();
	sceneCB = make_unique<UploadBuffer>();
	fogCB = make_unique<UploadBuffer>();

	shadowMgr = make_unique<ShadowMappingManager>();
	rtMgr = make_unique<RenderTargets>();
	lightMgr = make_unique<LightManager>();
	froxelMgr = make_unique<FroxelManager>();

	rootSig->Initialize(GetDevice());
	shader->InitializeAllShaders(GetDevice(), GetRootSig()->Get());
	frameCB->Initialize(GetDevice(), sizeof(FrameConstants));
	sceneCB->Initialize(GetDevice(), 256 * 1000);
	fogCB->Initialize(GetDevice(), sizeof(FogConstants));

	shadowMgr->Initialize(GetDevice());
	rtMgr->Initialize(GetDevice(), shadowMgr.get());
	lightMgr->Initialize(GetDevice());
	froxelMgr->Initialize(GetDevice());
}

void DX12Core::Update()
{
	// CSM Update
	shadowMgr->UpdateCascadeShadow(SCENE_MANAGER->GetCurrentScene()->GetCamera()->GetPosition());
}

void DX12Core::BeginShadowPass(int cascadeIdx)
{
	if (cascadeIdx == 0) {
		static bool firstShadowPass = true;

		if (!firstShadowPass) {
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				shadowMgr->GetCsmResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_DEPTH_WRITE
			);
			deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);
		}
		else {
			firstShadowPass = false;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE shadowDSV = shadowMgr->GetCsmDSV(cascadeIdx);
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSV);

	deviceCtx->GetGraphicsCmdList()->ClearDepthStencilView(shadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	UINT shadowMapSize = shadowMgr->GetShadowMapSize();
	D3D12_VIEWPORT shadowViewport = {};
	shadowViewport.Width = static_cast<float>(shadowMapSize);
	shadowViewport.Height = static_cast<float>(shadowMapSize);
	shadowViewport.MinDepth = 0.0f;
	shadowViewport.MaxDepth = 1.0f;
	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &shadowViewport);

	D3D12_RECT shadowRect = { 0, 0, static_cast<LONG>(shadowMapSize), static_cast<LONG>(shadowMapSize) };
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &shadowRect);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRoot32BitConstant(16, cascadeIdx, 0);

	//OutputDebugStringA("Shadow Pass started!!\n");
}

void DX12Core::EndShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, int cascadeIdx)
{
	if (cascadeIdx == shadowMgr->GetCascadeCount() - 1)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMgr->GetCsmResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);
	}

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);

	//OutputDebugStringA("Shadow Pass ended!!\n");
}

void DX12Core::BeginForwardPass()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetDepthBuffer(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChainMgr->GetCurrentRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = rtMgr->GetDSVHandle();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(4, lightMgr->GetForwardLightCB()->GetGPUVirtualAddress());

	/*FogConstants fog = { { 0.5f, 0.5f, 0.5f, 1.0f }, 2.0f, 3.5f, 0.0f, 20.0f, 6.0f, {0, 0, 0} };
	GetFogCB()->CopyData(&fog, sizeof(FogConstants));
	cmdList->SetGraphicsRootConstantBufferView(14, GetFogCB()->GetGPUVirtualAddress());*/

	//OutputDebugStringA("Forward pass started\n");
}

void DX12Core::BeginGBufferPass()
{
	static bool firstFrame = true;

	if (!firstFrame) {
		D3D12_RESOURCE_BARRIER barriers[3];
		for (int i = 0; i < 3; ++i) {
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
				rtMgr->GetGBuffer(i),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
		}
		deviceCtx->GetGraphicsCmdList()->ResourceBarrier(3, barriers);
	}
	else {
		firstFrame = false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = rtMgr->GetDSVHandle();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(3, rtMgr->GetGBufferRTVArray(), FALSE, &dsv);

	for (int i = 0; i < 3; ++i) {
		deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(rtMgr->GetGBufferRTV(i), clearColor, 0, nullptr);
	}

	deviceCtx->GetGraphicsCmdList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DX12Core::EndGBufferPass()
{
	D3D12_RESOURCE_BARRIER barriers[4];
	for (int i = 0; i < 3; ++i) {
		barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
			rtMgr->GetGBuffer(i),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	}
	barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetDepthBuffer(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(4, barriers);
}

void DX12Core::BeginLightingPass()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChainMgr->GetCurrentRTV();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(3, lightMgr->GetDeferredLightCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* heaps[] = { rtMgr->GetDeferredSRVHeap() };
	deviceCtx->GetGraphicsCmdList()->SetDescriptorHeaps(1, heaps);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(13, rtMgr->GetDeferredSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	/*FogConstants fog = { { 0.5f, 0.5f, 0.5f, 1.0f }, 2.0f, 3.5f, 0.0f, 20.0f, 6.0f, {0, 0, 0} };
	GetFogCB()->CopyData(&fog, sizeof(FogConstants));
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(14, GetFogCB()->GetGPUVirtualAddress());*/

	//OutputDebugStringA("Lighting Pass started\n");
}

void DX12Core::RenderFullscreenQuad()
{
	deviceCtx->GetGraphicsCmdList()->SetPipelineState(shader->GetPSO(PSOType::Lighting));

	deviceCtx->GetGraphicsCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceCtx->GetGraphicsCmdList()->DrawInstanced(6, 1, 0, 0);  

	//OutputDebugStringA("Fullscreen quad rendered\n");
}

void DX12Core::RenderBegin(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	deviceCtx->GetCmdAlloc()->Reset();
	deviceCtx->GetGraphicsCmdList()->Reset(deviceCtx->GetCmdAlloc(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainMgr->GetCurrentBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChainMgr->GetCurrentRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = rtMgr->GetDSVHandle();

	deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	deviceCtx->GetGraphicsCmdList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void DX12Core::RenderEnd()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainMgr->GetCurrentBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);
	deviceCtx->GetGraphicsCmdList()->Close();

	ID3D12CommandList* cmdListArr[] = { deviceCtx->GetGraphicsCmdList() };
	deviceCtx->GetCmdQueue()->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	HRESULT hr = swapChainMgr->Present();

	WaitSync();

	if (hr == DXGI_ERROR_INVALID_CALL || hr == DXGI_STATUS_OCCLUDED) {
		swapChainMgr->ResizeBuffers(GetDevice());
	}
}

void DX12Core::WaitSync()
{
	deviceCtx->WaitSync();
}

void DX12Core::FlushCommandQueue()
{
	deviceCtx->FlushCommandQueue();
}

void DX12Core::ResetCommandQueue()
{
	deviceCtx->ResetCommandQueue();
}

ID3D12Device* DX12Core::GetDevice() const
{
	return deviceCtx->GetDevice();
}

ID3D12CommandQueue* DX12Core::GetCmdQueue() const
{
	return deviceCtx->GetCmdQueue();
}

ID3D12GraphicsCommandList* DX12Core::GetGraphicsCmdList() const
{
	return deviceCtx->GetGraphicsCmdList();
}

ID3D12GraphicsCommandList* DX12Core::GetActiveCmdList() const
{
	return deviceCtx->GetActiveCmdList();
}

void DX12Core::SetLoadingMode(bool loading)
{
	deviceCtx->SetLoadingMode(loading);
}

void DX12Core::ExecuteLoadingCommands()
{
	deviceCtx->ExecuteLoadingCommands();
}

IDXGISwapChain4* DX12Core::GetSwapChain() const
{
	return swapChainMgr->GetSwapChain();
}

RootSignature* DX12Core::GetRootSig() const
{
	return rootSig.get();
}

Shader* DX12Core::GetShader() const
{
	return shader.get();
}

UploadBuffer* DX12Core::GetFrameCB() const
{
	return frameCB.get();
}

UploadBuffer* DX12Core::GetSceneCB() const
{
	return sceneCB.get();
}

UploadBuffer* DX12Core::GetFogCB() const
{
	return fogCB.get();
}

void DX12Core::SetPlayerPosForShadow(const XMFLOAT3& pos)
{
	playerCurrentPos = pos;
}
