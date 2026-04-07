#include "pch.h"
#include "UIManager.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>
#include "Engine.h"
#include "DX12Core.h"
#include "StartSceneUIController.h"
#include "GameSceneUIController.h"
#include "LoadingSceneUIController.h"

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

	// srcRGB * srcAlpha + destRGB * (1 - srcAlpha)
	CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	SpriteBatchPipelineStateDescription pd(rtState, &blendDesc);

	spriteBatch = make_unique<SpriteBatch>(core.GetDevice(), resourceUpload, pd, nullptr);

	RegisterFont(L"MalgunGothic", L"../Assets/UI/Fonts/MalgunGothic.spritefont", core, resourceUpload);	
	RegisterUITexture(L"MainPage", L"../Assets/UI/Textures/MainPage.png", core, resourceUpload);
	RegisterUITexture(L"PAB", L"../Assets/UI/Textures/PAB.png", core, resourceUpload);					
	RegisterUITexture(L"LOGIN", L"../Assets/UI/Textures/LOGIN.png", core, resourceUpload);
	RegisterUITexture(L"EXIT", L"../Assets/UI/Textures/EXIT.png", core, resourceUpload);

	RegisterUITexture(L"LoadingPage", L"../Assets/UI/Textures/LoadingPage.png", core, resourceUpload);
	RegisterUITexture(L"LoadingBarBack", L"../Assets/UI/Textures/LoadingBarBack.png", core, resourceUpload);
	RegisterUITexture(L"LoadingBar", L"../Assets/UI/Textures/LoadingBar.png", core, resourceUpload);
	RegisterUITexture(L"LoadingArrow", L"../Assets/UI/Textures/LoadingArrow.png", core, resourceUpload);

	RegisterUITexture(L"LocalCharBarsBack", L"../Assets/UI/Textures/LocalCharBarsBack.png", core, resourceUpload);
	RegisterUITexture(L"BarBack", L"../Assets/UI/Textures/BarBack.png", core, resourceUpload);
	RegisterUITexture(L"HpBar", L"../Assets/UI/Textures/HpBar.png", core, resourceUpload);
	RegisterUITexture(L"HpBar2", L"../Assets/UI/Textures/HpBar2.png", core, resourceUpload);
	RegisterUITexture(L"StaminaBar", L"../Assets/UI/Textures/StaminaBar.png", core, resourceUpload);

	RegisterUITexture(L"Status", L"../Assets/UI/Textures/Status.png", core, resourceUpload);			

	auto uploadFinished = resourceUpload.End(core.GetCmdQueue());
	uploadFinished.wait();

	RegisterControllers();
}

void UIManager::Update(float deltaTime)
{
	auto it = controllers.find(currentScene);
	if (it != controllers.end())
		it->second->Update(deltaTime);
}

void UIManager::Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp)
{
	ID3D12DescriptorHeap* heaps[] = { uiSrvHeap->Heap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	spriteBatch->SetViewport(vp);
	spriteBatch->Begin(cmdList);

	auto it = controllers.find(currentScene);
	if (it != controllers.end())
		it->second->Render(spriteBatch.get());

	spriteBatch->End();
	graphicsMemory->Commit(cmdQueue);
}

void UIManager::Release()
{
	controllers.clear();
	uiTextureMap.clear();
	uiFontMap.clear();
	spriteBatch.reset();
	uiSrvHeap.reset();
	graphicsMemory.reset();
}

UITextureData* UIManager::GetUITexture(const wstring& name)
{
	auto it = uiTextureMap.find(name);
	return (it != uiTextureMap.end()) ? &it->second : nullptr;
}

UIFontData* UIManager::GetFont(const wstring& name)
{
	auto it = uiFontMap.find(name);
	return (it != uiFontMap.end()) ? &it->second : nullptr;
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

void UIManager::RegisterControllers()
{
	controllers[SceneType::Title] = make_unique<StartSceneUIController>();
	controllers[SceneType::Title]->Init(this);

	controllers[SceneType::Village] = make_unique<GameSceneUIController>();
	controllers[SceneType::Village]->Init(this);

	controllers[SceneType::Loading] = make_unique<LoadingSceneUIController>();
	controllers[SceneType::Loading]->Init(this);
}
