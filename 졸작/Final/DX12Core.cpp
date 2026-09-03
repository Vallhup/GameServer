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
#include "SkyBox.h"
#include "ClusterLightManager.h"
#include "SSAO.h"
#include "LookUpTextures.h"
#include "Material.h"
#include "BloomManager.h"
#include "InstancingBatch.h"
#include "GameObject.h"
#include "Transform.h"
#include "SceneRenderer.h"

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
	volumetricFogCB = make_unique<UploadBuffer>();

	shadowMgr = make_unique<ShadowMappingManager>();
	rtMgr = make_unique<RenderTargets>();
	lightMgr = make_unique<LightManager>();
	clusterLightMgr = make_unique<ClusterLightManager>();
	ssaoMgr = make_unique<SSAO>();
	lutMgr = make_unique<LookUpTextures>();

	rootSig->Initialize(GetDevice());
	shader->InitializeAllShaders(GetDevice(), GetRootSig()->Get());
	frameCB->Initialize(GetDevice(), sizeof(FrameConstants));
	sceneCB->Initialize(GetDevice(), 256 * 1000);
	volumetricFogCB->Initialize(GetDevice(), sizeof(VolumetricFogConstants));
	volumetricFogData.texelSize = { 1.0f / WinSize.x, 1.0f / WinSize.y };
	volumetricFogCB->CopyData(&volumetricFogData, sizeof(VolumetricFogConstants));

	shadowMgr->Initialize(GetDevice());
	rtMgr->Initialize(GetDevice(), shadowMgr.get());
	lightMgr->Initialize(GetDevice());
	clusterLightMgr->Initialize(GetDevice());
	ssaoMgr->Initialize(GetDevice(), GetGraphicsCmdList(), GetRenderTargetMgr());
	rtMgr->AddSsaoSRV(GetDevice(), ssaoMgr->GetSsaoBlurRT());

	Material::InitializeBindlessSystem(GetDevice());
	rtMgr->RegisterHDRSceneToBindless(GetDevice());

	bloomMgr = make_unique<BloomManager>();
	bloomMgr->Initialize(GetDevice());
	bloomMgr->RegisterMipsToBindless(GetDevice());

	lutMgr->Initialize(GetDevice(), GetGraphicsCmdList());
}

void DX12Core::Update()
{
	// Final(성당) = 실내 → 태양 CSM 대신 overhead 동적 그림자(캐릭터 기준). 그 외는 일반 캐스케이드(카메라 타겟).
	if (SCENE_MANAGER->GetCurrentSceneType() == SceneType::Final)
		shadowMgr->UpdateOverheadShadow(playerCurrentPos);
	else
	{
		// Final을 떠나면 point shadow 비활성 + 재진입 시 재베이크
		if (shadowMgr->IsPointShadowBaked())
		{
			shadowMgr->SetPointShadowBaked(false);
			shadowMgr->GetCsmConstants().pointShadowCount = 0.0f;
		}
		shadowMgr->UpdateCascadeShadow(SCENE_MANAGER->GetCurrentScene()->GetCamera()->GetTargetPosition());
	}
}

void DX12Core::BeginShadowPass(int cascadeIdx)
{
	// cascade 0 진입 시: 슬라이스 0,1만 PSR→DW (슬라이스 2는 새 흐름이 별도 관리)
	if (cascadeIdx == 0) {
		static bool firstShadowPass = true;

		if (!firstShadowPass) {
			D3D12_RESOURCE_BARRIER barriers[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(
					shadowMgr->GetCsmResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					0),
				CD3DX12_RESOURCE_BARRIER::Transition(
					shadowMgr->GetCsmResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					1),
			};
			deviceCtx->GetGraphicsCmdList()->ResourceBarrier(2, barriers);
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
	// cascade 1 종료 시: 슬라이스 0,1만 DW→PSR (cascade 2는 새 흐름이 마무리)
	if (cascadeIdx == 1)
	{
		D3D12_RESOURCE_BARRIER barriers[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				shadowMgr->GetCsmResource(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				0),
			CD3DX12_RESOURCE_BARRIER::Transition(
				shadowMgr->GetCsmResource(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				1),
		};
		deviceCtx->GetGraphicsCmdList()->ResourceBarrier(2, barriers);
	}

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);

	//OutputDebugStringA("Shadow Pass ended!!\n");
}

void DX12Core::BeginStaticShadowPass()
{
	// staticCsm: 첫 프레임은 DW로 생성됨, 이후엔 COPY_SOURCE 상태
	static bool firstStaticPass = true;
	if (!firstStaticPass)
	{
		D3D12_RESOURCE_BARRIER toDw = CD3DX12_RESOURCE_BARRIER::Transition(
			shadowMgr->GetStaticCsmResource(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toDw);
	}
	else firstStaticPass = false;

	D3D12_CPU_DESCRIPTOR_HANDLE dsv = shadowMgr->GetStaticCsmDSV();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
	deviceCtx->GetGraphicsCmdList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	UINT mapSize = shadowMgr->GetShadowMapSize();
	D3D12_VIEWPORT vp = { 0, 0, (float)mapSize, (float)mapSize, 0.0f, 1.0f };
	D3D12_RECT rect = { 0, 0, (LONG)mapSize, (LONG)mapSize };
	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);

	int cascadeIdx = shadowMgr->GetStaticCacheCascadeIndex();
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRoot32BitConstant(16, cascadeIdx, 0);
}

void DX12Core::EndStaticShadowPass()
{
	D3D12_RESOURCE_BARRIER toCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMgr->GetStaticCsmResource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toCopySrc);
}

void DX12Core::CopyStaticToCsmCascade2()
{
	int cascadeIdx = shadowMgr->GetStaticCacheCascadeIndex();
	auto* cmd = deviceCtx->GetGraphicsCmdList();

	// csm 슬라이스 2: 첫 프레임은 DW(생성 시), 이후엔 PSR
	static bool firstCopy = true;
	D3D12_RESOURCE_STATES srcState = firstCopy
		? D3D12_RESOURCE_STATE_DEPTH_WRITE
		: D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	firstCopy = false;

	D3D12_RESOURCE_BARRIER toCopyDst = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMgr->GetCsmResource(),
		srcState,
		D3D12_RESOURCE_STATE_COPY_DEST,
		cascadeIdx);
	cmd->ResourceBarrier(1, &toCopyDst);

	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.pResource = shadowMgr->GetStaticCsmResource();
	src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = shadowMgr->GetCsmResource();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = cascadeIdx;

	cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	D3D12_RESOURCE_BARRIER toDw = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMgr->GetCsmResource(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		cascadeIdx);
	cmd->ResourceBarrier(1, &toDw);
}

void DX12Core::BeginDynamicShadowPass()
{
	// CopyStaticToCsmCascade2가 이미 슬라이스 2를 DW로 만들어놨음. clear는 안 함.
	int cascadeIdx = shadowMgr->GetStaticCacheCascadeIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE shadowDSV = shadowMgr->GetCsmDSV(cascadeIdx);
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSV);

	UINT mapSize = shadowMgr->GetShadowMapSize();
	D3D12_VIEWPORT vp = { 0, 0, (float)mapSize, (float)mapSize, 0.0f, 1.0f };
	D3D12_RECT rect = { 0, 0, (LONG)mapSize, (LONG)mapSize };
	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRoot32BitConstant(16, cascadeIdx, 0);
}

void DX12Core::EndDynamicShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	int cascadeIdx = shadowMgr->GetStaticCacheCascadeIndex();
	D3D12_RESOURCE_BARRIER toPsr = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMgr->GetCsmResource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		cascadeIdx);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toPsr);

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);
}

void DX12Core::BeginOverheadShadowPass()
{
	// 슬라이스 0만 사용. 직전 야외 씬이 슬라이스 0을 PSR로 남겨둔 상태에서 진입.
	D3D12_RESOURCE_BARRIER toDW = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMgr->GetCsmResource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		0);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toDW);

	D3D12_CPU_DESCRIPTOR_HANDLE shadowDSV = shadowMgr->GetCsmDSV(0);
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSV);
	deviceCtx->GetGraphicsCmdList()->ClearDepthStencilView(shadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	UINT mapSize = shadowMgr->GetShadowMapSize();
	D3D12_VIEWPORT vp = { 0, 0, (float)mapSize, (float)mapSize, 0.0f, 1.0f };
	D3D12_RECT rect = { 0, 0, (LONG)mapSize, (LONG)mapSize };
	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRoot32BitConstant(16, 0, 0);   // ShadowVS가 lightVP[0] 사용
}

void DX12Core::EndOverheadShadowPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	D3D12_RESOURCE_BARRIER toPsr = CD3DX12_RESOURCE_BARRIER::Transition(
		shadowMgr->GetCsmResource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		0);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toPsr);

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);
}

void DX12Core::BakePointShadows(const vector<shared_ptr<InstancingBatch>>& batches, SceneRenderer* renderer,
	const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	auto cmd = deviceCtx->GetGraphicsCmdList();

	int lightCount = lightMgr->GetDeferredLightData().lightCount;
	int pointCount = min(lightCount - 1, (int)ShadowMappingManager::POINT_SHADOW_MAX_LIGHTS);
	if (pointCount <= 0)
	{
		shadowMgr->SetPointShadowBaked(true);
		return;
	}

	shadowMgr->TransitionPointShadowToDepthWrite(cmd);

	const LightData* lights = lightMgr->GetLights();
	UploadBuffer* instancePool = shadowMgr->GetPointShadowInstancePool();
	constexpr size_t kInstancePoolCapacity = sizeof(XMMATRIX) * 65536;
	size_t instanceOffset = 0;

	UINT mapSize = ShadowMappingManager::POINT_SHADOW_MAP_SIZE;
	D3D12_VIEWPORT bakeVp = { 0, 0, (float)mapSize, (float)mapSize, 0.0f, 1.0f };
	D3D12_RECT bakeRect = { 0, 0, (LONG)mapSize, (LONG)mapSize };
	cmd->RSSetViewports(1, &bakeVp);
	cmd->RSSetScissorRects(1, &bakeRect);

	cmd->SetGraphicsRootSignature(GetRootSig()->Get());
	Material::BindBindlessResources(cmd);   // ShadowPS의 알파텍스처/materialBuffer
	cmd->SetGraphicsRootConstantBufferView(0, frameCB->GetGPUVirtualAddress());
	cmd->SetGraphicsRoot32BitConstant(16, 0, 0);   // ShadowVS가 lightVP[0] 사용
	cmd->SetPipelineState(shader->GetPSO(PSOType::PointShadow));

	struct DrawChunk { InstancingBatch* batch; D3D12_GPU_VIRTUAL_ADDRESS va; UINT count; };
	vector<DrawChunk> chunks;
	vector<XMMATRIX> casterMatrices;

	for (int li = 0; li < pointCount; ++li)
	{
		const LightData& light = lights[li + 1];
		if (light.type != 1 || light.intensity <= 0.0f || light.range <= 0.0f)
			continue;

		BoundingSphere lightSphere(light.position, light.range);

		// 라이트 영향권의 정적 캐스터 수집 (OBB vs range 구체) — 모든 면이 같은 캐스터 셋 공유
		chunks.clear();
		for (const auto& batch : batches)
		{
			if (!batch->IsCastShadow())
				continue;

			casterMatrices.clear();
			for (const auto& obj : batch->GetObjects())
			{
				const BoundingOrientedBox& obb = obj->GetWorldBoundingBox();
				bool hit;
				if (obb.Extents.x > 0.0f)
					hit = obb.Intersects(lightSphere);
				else
				{
					const XMFLOAT3& p = obj->GetComponent<Transform>()->GetPosition();
					float dx = p.x - light.position.x, dy = p.y - light.position.y, dz = p.z - light.position.z;
					float reach = light.range + 20.0f;   // 바운딩 없는 인스턴스는 거리 기반 보수적 포함
					hit = (dx * dx + dy * dy + dz * dz) < reach * reach;
				}
				if (hit)
					casterMatrices.push_back(XMMatrixTranspose(obj->GetComponent<Transform>()->GetWorldMatrix()));
			}

			if (casterMatrices.empty())
				continue;

			size_t bytes = sizeof(XMMATRIX) * casterMatrices.size();
			if (instanceOffset + bytes > kInstancePoolCapacity)
			{
				OutputDebugStringA("Point shadow bake instance pool overflowed!!\n");
				break;
			}

			instancePool->CopyData(casterMatrices.data(), bytes, instanceOffset);
			chunks.push_back({ batch.get(), instancePool->GetGPUVirtualAddress() + instanceOffset,
				static_cast<UINT>(casterMatrices.size()) });
			instanceOffset += bytes;
		}

		for (int face = 0; face < 6; ++face)
		{
			int slice = li * 6 + face;
			D3D12_CPU_DESCRIPTOR_HANDLE dsv = shadowMgr->GetPointShadowDSV(slice);
			cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
			cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			if (chunks.empty())
				continue;

			cmd->SetGraphicsRootConstantBufferView(5,
				shadowMgr->WritePointShadowFaceCB(slice, light.position, face, light.range));

			for (const auto& chunk : chunks)
				renderer->RenderPointShadowChunk(*this, chunk.batch, chunk.va, chunk.count);
		}
	}

	shadowMgr->TransitionPointShadowToShaderResource(cmd);

	shadowMgr->GetCsmConstants().pointShadowCount = static_cast<float>(pointCount);
	shadowMgr->UploadCsmConstants();   // 이번 프레임 LightingPass부터 바로 반영
	shadowMgr->SetPointShadowBaked(true);

	cmd->RSSetViewports(1, &vp);
	cmd->RSSetScissorRects(1, &rect);

	OutputDebugStringA(("Point shadows baked: " + to_string(pointCount) + " lights, "
		+ to_string(instanceOffset / sizeof(XMMATRIX)) + " caster instances\n").c_str());
}

void DX12Core::ForwardPass()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetDepthBuffer(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtMgr->GetHDRSceneRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = rtMgr->GetDSVHandle();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootShaderResourceView(25, lightMgr->GetDeferredLightSB()->GetGPUVirtualAddress());

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(22, GetVolumetricFogCB()->GetGPUVirtualAddress());

	// Shadow Map SRV 바인딩 (t4-t9 테이블, Root Index 13)
	ID3D12DescriptorHeap* heaps[] = { rtMgr->GetDeferredSRVHeap() };
	deviceCtx->GetGraphicsCmdList()->SetDescriptorHeaps(1, heaps);
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(13, rtMgr->GetDeferredSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	//OutputDebugStringA("Forward pass started\n");
}

void DX12Core::BloomPass()
{
	auto* cmd = deviceCtx->GetGraphicsCmdList();
	auto* bloomTex = bloomMgr->GetBloomTexture();

	// HDR Scene : RENDER_TARGET -> PIXEL_SHADER_RESOURCE (source for downsample[0])
	{
		D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::Transition(
			rtMgr->GetHDRSceneRT(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd->ResourceBarrier(1, &b);
	}

	cmd->SetGraphicsRootSignature(GetRootSig()->Get());
	Material::BindBindlessResources(cmd);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ===== Downsample : HDR -> mip0, mip0 -> mip1, ... =====
	cmd->SetPipelineState(shader->GetPSO(PSOType::BloomDownsample));

	for (UINT i = 0; i < BloomManager::CHAIN_LENGTH; ++i)
	{
		D3D12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
			bloomTex,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET, i);
		cmd->ResourceBarrier(1, &toRT);

		UINT srcIdx = (i == 0) ? Material::HDR_SCENE_BINDLESS_INDEX : (Material::BLOOM_MIP_BASE + (i - 1));
		UINT srcW = (i == 0) ? static_cast<UINT>(WinSize.x) : bloomMgr->GetMipWidth(i - 1);
		UINT srcH = (i == 0) ? static_cast<UINT>(WinSize.y) : bloomMgr->GetMipHeight(i - 1);

		BloomRootConstants rc = {};
		rc.srcMipIndex = srcIdx;
		rc.filterRadius = 0.0f;
		rc.texelSizeX = 1.0f / static_cast<float>(srcW);
		rc.texelSizeY = 1.0f / static_cast<float>(srcH);
		rc.intensity = 1.0f;
		rc.threshold = 1.0f;
		rc.knee = 0.1f;
		rc.isFirstPass = (i == 0) ? 1u : 0u;
		cmd->SetGraphicsRoot32BitConstants(24, 8, &rc, 0);

		D3D12_VIEWPORT vp = {};
		vp.Width = static_cast<FLOAT>(bloomMgr->GetMipWidth(i));
		vp.Height = static_cast<FLOAT>(bloomMgr->GetMipHeight(i));
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		cmd->RSSetViewports(1, &vp);

		D3D12_RECT sc = { 0, 0,
			static_cast<LONG>(bloomMgr->GetMipWidth(i)),
			static_cast<LONG>(bloomMgr->GetMipHeight(i)) };
		cmd->RSSetScissorRects(1, &sc);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = bloomMgr->GetMipRTV(i);
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		cmd->DrawInstanced(6, 1, 0, 0);

		D3D12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
			bloomTex,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, i);
		cmd->ResourceBarrier(1, &toSRV);
	}

	// ===== Upsample (additive) : mip5 -> mip4, mip4 -> mip3, ..., mip1 -> mip0 =====
	cmd->SetPipelineState(shader->GetPSO(PSOType::BloomUpsample));

	for (int i = static_cast<int>(BloomManager::CHAIN_LENGTH) - 1; i > 0; --i)
	{
		UINT dstMip = static_cast<UINT>(i - 1);

		D3D12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
			bloomTex,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET, dstMip);
		cmd->ResourceBarrier(1, &toRT);

		BloomRootConstants rc = {};
		rc.srcMipIndex = Material::BLOOM_MIP_BASE + static_cast<UINT>(i);
		rc.filterRadius = 0.005f;
		rc.texelSizeX = 1.0f / static_cast<float>(bloomMgr->GetMipWidth(i));
		rc.texelSizeY = 1.0f / static_cast<float>(bloomMgr->GetMipHeight(i));
		rc.intensity = 1.0f;
		cmd->SetGraphicsRoot32BitConstants(24, 8, &rc, 0);

		D3D12_VIEWPORT vp = {};
		vp.Width = static_cast<FLOAT>(bloomMgr->GetMipWidth(dstMip));
		vp.Height = static_cast<FLOAT>(bloomMgr->GetMipHeight(dstMip));
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		cmd->RSSetViewports(1, &vp);

		D3D12_RECT sc = { 0, 0,
			static_cast<LONG>(bloomMgr->GetMipWidth(dstMip)),
			static_cast<LONG>(bloomMgr->GetMipHeight(dstMip)) };
		cmd->RSSetScissorRects(1, &sc);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = bloomMgr->GetMipRTV(dstMip);
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		cmd->DrawInstanced(6, 1, 0, 0);

		D3D12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
			bloomTex,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, dstMip);
		cmd->ResourceBarrier(1, &toSRV);
	}

	// HDR Scene back to RENDER_TARGET so BlitPass's standard RT->SRV transition stays consistent.
	{
		D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::Transition(
			rtMgr->GetHDRSceneRT(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd->ResourceBarrier(1, &b);
	}

	D3D12_VIEWPORT fullVP = { 0, 0, static_cast<FLOAT>(WinSize.x), static_cast<FLOAT>(WinSize.y), 0.0f, 1.0f };
	cmd->RSSetViewports(1, &fullVP);
	D3D12_RECT fullSC = { 0, 0, static_cast<LONG>(WinSize.x), static_cast<LONG>(WinSize.y) };
	cmd->RSSetScissorRects(1, &fullSC);
}

void DX12Core::BlitPass()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetHDRSceneRT(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChainMgr->GetCurrentRTV();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetPipelineState(shader->GetPSO(PSOType::Blit));

	Material::BindBindlessResources(deviceCtx->GetGraphicsCmdList());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());

	// Bind bloom intensity (other BloomCB fields unused by Blit)
	BloomRootConstants blitRC = {};
	blitRC.srcMipIndex = Material::BLOOM_MIP_BASE;
	blitRC.filterRadius = 0.0f;
	blitRC.texelSizeX = 0.0f;
	blitRC.texelSizeY = 0.0f;
	blitRC.intensity = bloomMgr->GetIntensity();
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRoot32BitConstants(24, 8, &blitRC, 0);

	deviceCtx->GetGraphicsCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceCtx->GetGraphicsCmdList()->DrawInstanced(6, 1, 0, 0);

	D3D12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetHDRSceneRT(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toRT);
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

void DX12Core::SsaoPass()
{
	static bool firstFrame = true;
	if (!firstFrame) {
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			ssaoMgr->GetSsaoRT(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);
	}
	else {
		firstFrame = false;
	}

	D3D12_VIEWPORT ssaoViewport = {};
	ssaoViewport.Width = WinSize.x / 2.0f;
	ssaoViewport.Height = WinSize.y / 2.0f;
	ssaoViewport.MinDepth = 0.0f;
	ssaoViewport.MaxDepth = 1.0f;
	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &ssaoViewport);

	D3D12_RECT ssaoRect = { 0, 0, static_cast<LONG>(WinSize.x / 2), static_cast<LONG>(WinSize.y / 2) };
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &ssaoRect);

	D3D12_CPU_DESCRIPTOR_HANDLE ssaoRTV = ssaoMgr->GetSsaoRTVHandle();
	float clearValue[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(ssaoRTV, clearValue, 0, nullptr);
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &ssaoRTV, FALSE, nullptr);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetPipelineState(shader->GetPSO(PSOType::Ssao));

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(17, ssaoMgr->GetSsaoCB()->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* heaps[] = { ssaoMgr->GetSsaoSRVHeap() };
	deviceCtx->GetGraphicsCmdList()->SetDescriptorHeaps(1, heaps);
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(18, ssaoMgr->GetSsaoSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	deviceCtx->GetGraphicsCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceCtx->GetGraphicsCmdList()->DrawInstanced(6, 1, 0, 0);

	D3D12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
		ssaoMgr->GetSsaoRT(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toSRV);
}

void DX12Core::SsaoBlurPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		ssaoMgr->GetSsaoBlurRT(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE ssaoBlurRTV = ssaoMgr->GetSsaoBlurRTVHandle();
	float clearValue[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(ssaoBlurRTV, clearValue, 0, nullptr);
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &ssaoBlurRTV, FALSE, nullptr);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetPipelineState(shader->GetPSO(PSOType::SsaoBlur));

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(17, ssaoMgr->GetSsaoCB()->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* heaps[] = { ssaoMgr->GetSsaoSRVHeap() };
	deviceCtx->GetGraphicsCmdList()->SetDescriptorHeaps(1, heaps);
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(18, ssaoMgr->GetSsaoSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	deviceCtx->GetGraphicsCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceCtx->GetGraphicsCmdList()->DrawInstanced(6, 1, 0, 0);

	D3D12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
		ssaoMgr->GetSsaoBlurRT(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toSRV);

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);
}

void DX12Core::ClearSsaoRT()
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		ssaoMgr->GetSsaoBlurRT(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);
	

	D3D12_CPU_DESCRIPTOR_HANDLE blurRTV = ssaoMgr->GetSsaoBlurRTVHandle();  
	float clearValue[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(blurRTV, clearValue, 0, nullptr);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		ssaoMgr->GetSsaoBlurRT(),  
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);
}

void DX12Core::FogPass(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetFogRT(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &barrier);

	D3D12_VIEWPORT fogViewport = {};
	fogViewport.Width = WinSize.x;
	fogViewport.Height = WinSize.y;
	fogViewport.MinDepth = 0.0f;
	fogViewport.MaxDepth = 1.0f;
	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &fogViewport);

	D3D12_RECT fogRect = { 0, 0, static_cast<LONG>(WinSize.x), static_cast<LONG>(WinSize.y) };
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &fogRect);

	D3D12_CPU_DESCRIPTOR_HANDLE fogRTV = rtMgr->GetFogRTV();
	float clearValue[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(fogRTV, clearValue, 0, nullptr);
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &fogRTV, FALSE, nullptr);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());
	deviceCtx->GetGraphicsCmdList()->SetPipelineState(shader->GetPSO(PSOType::VolumetricFogPass));

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootShaderResourceView(25, lightMgr->GetDeferredLightSB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(22, GetVolumetricFogCB()->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* heaps[] = { rtMgr->GetDeferredSRVHeap() };
	deviceCtx->GetGraphicsCmdList()->SetDescriptorHeaps(1, heaps);
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(13, rtMgr->GetDeferredSRVHeap()->GetGPUDescriptorHandleForHeapStart());

	deviceCtx->GetGraphicsCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceCtx->GetGraphicsCmdList()->DrawInstanced(6, 1, 0, 0);

	D3D12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
		rtMgr->GetFogRT(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(1, &toSRV);

	deviceCtx->GetGraphicsCmdList()->RSSetViewports(1, &vp);
	deviceCtx->GetGraphicsCmdList()->RSSetScissorRects(1, &rect);
}

void DX12Core::ClusterLightCullPass()
{
	auto* cmd = deviceCtx->GetGraphicsCmdList();

	// 1. globalCounter 0 클리어 (자체 디스크립터 힙 set)
	clusterLightMgr->ClearCounter(cmd);

	// 2. compute pipeline + root signature
	cmd->SetComputeRootSignature(GetRootSig()->Get());
	cmd->SetPipelineState(shader->GetPSO(PSOType::ClusterLightCull));

	// 3. root parameter 바인딩 (compute 슬롯)
	cmd->SetComputeRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	cmd->SetComputeRootConstantBufferView(3, lightMgr->GetDeferredLightCB()->GetGPUVirtualAddress());
	cmd->SetComputeRootShaderResourceView(25, lightMgr->GetDeferredLightSB()->GetGPUVirtualAddress());
	cmd->SetComputeRootConstantBufferView(26, clusterLightMgr->GetParamsCB()->GetGPUVirtualAddress());
	cmd->SetComputeRootUnorderedAccessView(29, clusterLightMgr->GetLightIndexList()->GetGPUVirtualAddress());
	cmd->SetComputeRootUnorderedAccessView(30, clusterLightMgr->GetLightGrid()->GetGPUVirtualAddress());
	cmd->SetComputeRootUnorderedAccessView(31, clusterLightMgr->GetGlobalCounter()->GetGPUVirtualAddress());

	// 4. dispatch — thread group 1개 = cluster 1개
	cmd->Dispatch(ClusterLightManager::GRID_X, ClusterLightManager::GRID_Y, ClusterLightManager::GRID_Z);

	// 5. UAV → SRV (LightingPass 의 PS read 준비)
	D3D12_RESOURCE_BARRIER toSRV[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(
			clusterLightMgr->GetLightIndexList()->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(
			clusterLightMgr->GetLightGrid()->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	cmd->ResourceBarrier(_countof(toSRV), toSRV);
}

void DX12Core::LightingPass()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtMgr->GetHDRSceneRTV();
	deviceCtx->GetGraphicsCmdList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	float clearValue[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	deviceCtx->GetGraphicsCmdList()->ClearRenderTargetView(rtv, clearValue, 0, nullptr);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootSignature(GetRootSig()->Get());

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(0, GetFrameCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(3, lightMgr->GetDeferredLightCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(5, shadowMgr->GetCsmCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootShaderResourceView(25, lightMgr->GetDeferredLightSB()->GetGPUVirtualAddress());

	// Clustered Shading — PS lookup
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(26, clusterLightMgr->GetParamsCB()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootShaderResourceView(27, clusterLightMgr->GetLightIndexList()->GetGPUVirtualAddress());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootShaderResourceView(28, clusterLightMgr->GetLightGrid()->GetGPUVirtualAddress());

	if (auto* sky = lightMgr->GetSkyBox())
		deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(20, sky->GetCBAddress());

	ID3D12DescriptorHeap* heaps[] = { rtMgr->GetDeferredSRVHeap() };
	deviceCtx->GetGraphicsCmdList()->SetDescriptorHeaps(1, heaps);

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(13, rtMgr->GetDeferredSRVHeap()->GetGPUDescriptorHandleForHeapStart());
	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootDescriptorTable(4, rtMgr->GetPointShadowSRV());

	deviceCtx->GetGraphicsCmdList()->SetGraphicsRootConstantBufferView(22, GetVolumetricFogCB()->GetGPUVirtualAddress());

	deviceCtx->GetGraphicsCmdList()->SetPipelineState(shader->GetPSO(PSOType::Lighting));

	deviceCtx->GetGraphicsCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceCtx->GetGraphicsCmdList()->DrawInstanced(6, 1, 0, 0);
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
	// cluster UAV 복원 (다음 프레임 cull dispatch 위해 SRV → UAV)
	D3D12_RESOURCE_BARRIER clusterToUAV[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(
			clusterLightMgr->GetLightIndexList()->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(
			clusterLightMgr->GetLightGrid()->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
	};
	deviceCtx->GetGraphicsCmdList()->ResourceBarrier(_countof(clusterToUAV), clusterToUAV);

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

UploadBuffer* DX12Core::GetVolumetricFogCB() const
{
	return volumetricFogCB.get();
}

void DX12Core::UpdateVolumetricFog()
{
	volumetricFogCB->CopyData(&volumetricFogData, sizeof(VolumetricFogConstants));
}

void DX12Core::SetPlayerPosForShadow(const XMFLOAT3& pos)
{
	playerCurrentPos = pos;
}
