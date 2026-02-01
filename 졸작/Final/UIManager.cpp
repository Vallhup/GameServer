#include "pch.h"
#include "UIManager.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>
#include "DX12Core.h"
#include "Input.h"

UINT UIManager::nextIndex = 0;

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

	// Registering Font
	RegisterFont(L"MalgunGothic", L"../Assets/UI/Fonts/MalgunGothic.spritefont", core, resourceUpload);

	// Registering Texture
	RegisterUITexture(L"Status", L"../Assets/UI/Textures/Status.png", core, resourceUpload);

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

	if (GET(Input).GetKeyDown('K'))
		status = !status;

	if (status)
	{
		auto& statusTex = uiTextureMap[L"Status"];

		XMUINT2 texSize = GetTextureSize(statusTex.resource.Get());
		RECT destRect = { 0, 0, static_cast<LONG>(texSize.x * 0.5f), static_cast<LONG>(texSize.y * 0.5f) };
		spriteBatch->Draw(uiSrvHeap->GetGpuHandle(statusTex.heapIndex), texSize, destRect);
	}

	auto& font = uiFontMap[L"MalgunGothic"].font;
	XMVECTOR textSize = font->MeasureString(L"Hello, I'm JeongHo Lee");
	XMFLOAT2 origin(XMVectorGetX(textSize) / 2.f, XMVectorGetY(textSize) / 2.f);
	XMFLOAT2 pos(vp.Width / 2.f, vp.Height / 2.f);
	font->DrawString(spriteBatch.get(), L"Hello, I'm JeongHo Lee", pos, Colors::White, 0.f, origin);

	spriteBatch->End();

	graphicsMemory->Commit(cmdQueue);
}

void UIManager::Release()
{
	uiTextureMap.clear();
	uiFontMap.clear();
	spriteBatch.reset();
	uiSrvHeap.reset();
	graphicsMemory.reset();
}

void UIManager::RegisterFont(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload)
{
	if (nextIndex >= 31) return;
	if (uiFontMap.find(name) != uiFontMap.end()) return;

	auto& font = uiFontMap[name];
	font.heapIndex = nextIndex;
	font.font = make_unique<SpriteFont>(core.GetDevice(), upload, path, 
		uiSrvHeap->GetCpuHandle(font.heapIndex), uiSrvHeap->GetGpuHandle(font.heapIndex));

	nextIndex++;
}

void UIManager::RegisterUITexture(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload)
{
	if (nextIndex >= 31) return;
	if (uiTextureMap.find(name) != uiTextureMap.end()) return;

	auto& tex = uiTextureMap[name];
	tex.heapIndex = nextIndex;

	CreateWICTextureFromFile(core.GetDevice(), upload, path, tex.resource.ReleaseAndGetAddressOf());
	CreateShaderResourceView(core.GetDevice(), tex.resource.Get(), uiSrvHeap->GetCpuHandle(tex.heapIndex));
	
	nextIndex++;
}
