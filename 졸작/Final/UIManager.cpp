#include "pch.h"
#include "UIManager.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>
#include "Engine.h"
#include "DX12Core.h"
#include "StartSceneUIController.h"
#include "SelectSceneUIController.h"
#include "GameSceneUIController.h"
#include "LoadingSceneUIController.h"

UINT UIManager::nextIndex = 0;

void UIManager::Initialize(DX12Core& core)
{
	graphicsMemory = make_unique<GraphicsMemory>(core.GetDevice());

	uiSrvHeap = make_unique<DescriptorHeap>(core.GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, MAX_RESOURCE_COUNT);

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
	RegisterUITexture(L"LoadingPlaza", L"../Assets/UI/Textures/LoadingPlaza.png", core, resourceUpload);
	RegisterUITexture(L"LoadingBarBack", L"../Assets/UI/Textures/LoadingBarBack.png", core, resourceUpload);
	RegisterUITexture(L"LoadingBar", L"../Assets/UI/Textures/LoadingBar.png", core, resourceUpload);
	RegisterUITexture(L"LoadingArrow", L"../Assets/UI/Textures/LoadingArrow.png", core, resourceUpload);

	RegisterUITexture(L"LocalCharBarsBack", L"../Assets/UI/Textures/LocalCharBarsBack.png", core, resourceUpload);
	RegisterUITexture(L"BarBack", L"../Assets/UI/Textures/BarBack.png", core, resourceUpload);
	RegisterUITexture(L"HpBar", L"../Assets/UI/Textures/HpBar.png", core, resourceUpload);
	RegisterUITexture(L"HpBar2", L"../Assets/UI/Textures/HpBar2.png", core, resourceUpload);
	RegisterUITexture(L"StaminaBar", L"../Assets/UI/Textures/StaminaBar.png", core, resourceUpload);
	RegisterUITexture(L"PressAnyButton", L"../Assets/UI/Textures/PRB.png", core, resourceUpload);

	RegisterUITexture(L"CharBackground", L"../Assets/UI/Textures/charBackground.png", core, resourceUpload);
	RegisterUITexture(L"CharKnight", L"../Assets/UI/Textures/CharKnight.png", core, resourceUpload);
	RegisterUITexture(L"CharLancer", L"../Assets/UI/Textures/CharLancer.png", core, resourceUpload);
	RegisterUITexture(L"CharPaladin", L"../Assets/UI/Textures/CharPaladin.png", core, resourceUpload);
	RegisterUITexture(L"CharHover", L"../Assets/UI/Textures/CharHover.png", core, resourceUpload);

	RegisterUITexture(L"PlazaName", L"../Assets/UI/Textures/PlazaName.png", core, resourceUpload);
	RegisterUITexture(L"VillageName", L"../Assets/UI/Textures/VillageName.png", core, resourceUpload);
	RegisterUITexture(L"CastleName", L"../Assets/UI/Textures/CastleName.png", core, resourceUpload);
	RegisterUITexture(L"FinalName", L"../Assets/UI/Textures/FinalName.png", core, resourceUpload);

	RegisterUITexture(L"Status", L"../Assets/UI/Textures/Status.png", core, resourceUpload);
	RegisterUITexture(L"StatusBack", L"../Assets/UI/Textures/StatusBack.png", core, resourceUpload);
	RegisterUITexture(L"StatusRibbon",     L"../Assets/UI/Textures/StatusRibbon.png", core, resourceUpload);
	RegisterUITexture(L"StatusArrowLeft",  L"../Assets/UI/Textures/StatusArrowLeft.png", core, resourceUpload);
	RegisterUITexture(L"StatusArrowRight", L"../Assets/UI/Textures/StatusArrowRight.png", core, resourceUpload);

	RegisterUITexture(L"Black", L"../Assets/UI/Textures/Black.png", core, resourceUpload);
	RegisterUITexture(L"PlazaMap", L"../Assets/UI/Textures/PlazaMap.png", core, resourceUpload);
	RegisterUITexture(L"VillageMap", L"../Assets/UI/Textures/VillageMap.png", core, resourceUpload);
	RegisterUITexture(L"CastleMap", L"../Assets/UI/Textures/CastleMap.png", core, resourceUpload);

	RegisterUITexture(L"PartyBook", L"../Assets/UI/Textures/PartyBook.png", core, resourceUpload);
	RegisterUITexture(L"PartyList", L"../Assets/UI/Textures/PartyList.png", core, resourceUpload);
	RegisterUITexture(L"PartyCreate", L"../Assets/UI/Textures/PartyCreate.png", core, resourceUpload);
	RegisterUITexture(L"PartyJoin", L"../Assets/UI/Textures/PartyJoin.png", core, resourceUpload);
	RegisterUITexture(L"PartyMyParty", L"../Assets/UI/Textures/PartyMyParty.png", core, resourceUpload);
	RegisterUITexture(L"PartyBack", L"../Assets/UI/Textures/PartyBack.png", core, resourceUpload);

	RegisterUITexture(L"EscWindow", L"../Assets/UI/Textures/EscWindow.png", core, resourceUpload);
	RegisterUITexture(L"ESCContinue", L"../Assets/UI/Textures/ESCContinue.png", core, resourceUpload);
	RegisterUITexture(L"ESCSetting", L"../Assets/UI/Textures/ESCSetting.png", core, resourceUpload);
	RegisterUITexture(L"ESCQuit", L"../Assets/UI/Textures/ESCQuit.png", core, resourceUpload);

	RegisterUITexture(L"KeyGuide", L"../Assets/UI/Textures/KeyGuide.png", core, resourceUpload);

	RegisterUITexture(L"SettingWindow", L"../Assets/UI/Textures/SettingWindow.png", core, resourceUpload);

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
	if (nextIndex >= MAX_RESOURCE_COUNT - 1) return;
	if (uiFontMap.find(name) != uiFontMap.end()) return;

	auto& font = uiFontMap[name];
	font.heapIndex = nextIndex;
	font.font = make_unique<SpriteFont>(core.GetDevice(), upload, path, 
		uiSrvHeap->GetCpuHandle(font.heapIndex), uiSrvHeap->GetGpuHandle(font.heapIndex));

	nextIndex++;
}

void UIManager::RegisterUITexture(const wstring& name, const wchar_t* path, DX12Core& core, ResourceUploadBatch& upload)
{
	if (nextIndex >= MAX_RESOURCE_COUNT - 1) return;
	if (uiTextureMap.find(name) != uiTextureMap.end()) return;

	auto& tex = uiTextureMap[name];
	tex.heapIndex = nextIndex;

	CreateWICTextureFromFileEx(core.GetDevice(), upload, path, 0, D3D12_RESOURCE_FLAG_NONE, WIC_LOADER_FORCE_SRGB, tex.resource.ReleaseAndGetAddressOf());
	CreateShaderResourceView(core.GetDevice(), tex.resource.Get(), uiSrvHeap->GetCpuHandle(tex.heapIndex));
	
	nextIndex++;
}

void UIManager::RegisterControllers()
{
	controllers[SceneType::Title] = make_unique<StartSceneUIController>();
	controllers[SceneType::Title]->Init(this);

	controllers[SceneType::Select] = make_unique<SelectSceneUIController>();
	controllers[SceneType::Select]->Init(this);

	controllers[SceneType::Plaza] = make_unique<GameSceneUIController>(SceneType::Plaza);
	controllers[SceneType::Plaza]->Init(this);

	controllers[SceneType::Village] = make_unique<GameSceneUIController>(SceneType::Village);
	controllers[SceneType::Village]->Init(this);

	controllers[SceneType::Castle] = make_unique<GameSceneUIController>(SceneType::Castle);
	controllers[SceneType::Castle]->Init(this);

	controllers[SceneType::Final] = make_unique<GameSceneUIController>(SceneType::Final);
	controllers[SceneType::Final]->Init(this);

	controllers[SceneType::Loading] = make_unique<LoadingSceneUIController>();
	controllers[SceneType::Loading]->Init(this);
}
