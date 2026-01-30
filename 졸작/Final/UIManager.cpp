#include "pch.h"
#include "UIManager.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>
#include "DX12Core.h"
#include "Input.h"

void UIManager::Initialize(DX12Core& core)
{
	graphicsMemory = make_unique<GraphicsMemory>(core.GetDevice());

	uiSrvHeap = make_unique<DescriptorHeap>(
		core.GetDevice(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		32);

	RenderTargetState rtState(
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DXGI_FORMAT_D32_FLOAT);

	ResourceUploadBatch resourceUpload(core.GetDevice());
	resourceUpload.Begin();

	SpriteBatchPipelineStateDescription pd(rtState);

	spriteBatch = make_unique<SpriteBatch>(core.GetDevice(), resourceUpload, pd, nullptr);
	spriteFont = make_unique<SpriteFont>(core.GetDevice(), resourceUpload, L"../Assets/UI/Fonts/MalgunGothic.spritefont",
		uiSrvHeap->GetCpuHandle(0), uiSrvHeap->GetGpuHandle(0));

	CreateWICTextureFromFile(core.GetDevice(), resourceUpload, L"../Assets/UI/Textures/Status.png",
		statusTexture.ReleaseAndGetAddressOf());
	CreateShaderResourceView(core.GetDevice(), statusTexture.Get(),
		uiSrvHeap->GetCpuHandle(1));

	auto uploadFinished = resourceUpload.End(core.GetCmdQueue());
	uploadFinished.wait();
}

void UIManager::Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp)
{
	ID3D12DescriptorHeap* heaps[] = { uiSrvHeap->Heap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	spriteBatch->SetViewport(vp);
	spriteBatch->Begin(cmdList);

	static bool status = true;

	if (GET(Input).GetKeyDown(VK_F2))
		status = !status;

	if (status)
	{
		XMUINT2 texSize = GetTextureSize(statusTexture.Get());
		RECT destRect = { 0, 0, static_cast<LONG>(texSize.x * 0.5f), static_cast<LONG>(texSize.y * 0.5f) };
		spriteBatch->Draw(uiSrvHeap->GetGpuHandle(1), texSize, destRect);
	}

	XMVECTOR textSize = spriteFont->MeasureString(L"Hello, I'm JeongHo Lee");
	XMFLOAT2 origin(XMVectorGetX(textSize) / 2.f, XMVectorGetY(textSize) / 2.f);
	XMFLOAT2 pos(vp.Width / 2.f, vp.Height / 2.f);

	spriteFont->DrawString(spriteBatch.get(), L"Hello, I'm JeongHo Lee", pos, Colors::White, 0.f, origin);

	spriteBatch->End();

	graphicsMemory->Commit(cmdQueue);
}

void UIManager::Release()
{
	statusTexture.Reset();
	spriteFont.reset();
	spriteBatch.reset();
	uiSrvHeap.reset();
	graphicsMemory.reset();
}