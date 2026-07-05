#include "pch.h"
#include "UIManager.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>
#include "Engine.h"
#include "DX12Core.h"
#include "VideoPlayer.h"
#include "StartSceneUIController.h"
#include "SelectSceneUIController.h"
#include "GameSceneUIController.h"
#include "LoadingSceneUIController.h"
#include "SoundManager.h"

UINT UIManager::nextIndex = 0;

UIManager::UIManager() = default;
UIManager::~UIManager() = default;

void UIManager::Initialize(DX12Core& core)
{
	coreRef = &core;

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
	RegisterFont(L"VerdanaBold", L"../Assets/UI/Fonts/VerdanaBold.spritefont", core, resourceUpload);

	RegisterUITexture(L"MainPage", L"../Assets/UI/Textures/MainPage.png", core, resourceUpload);
	RegisterUITexture(L"PressAnyButton", L"../Assets/UI/Textures/PRB.png", core, resourceUpload);
	RegisterUITexture(L"LOGIN", L"../Assets/UI/Textures/LOGIN.png", core, resourceUpload);
	RegisterUITexture(L"EXIT", L"../Assets/UI/Textures/EXIT.png", core, resourceUpload);

	RegisterUITexture(L"LoadingPage", L"../Assets/UI/Textures/LoadingPage.png", core, resourceUpload);
	RegisterUITexture(L"LoadingPlaza", L"../Assets/UI/Textures/LoadingPlaza.png", core, resourceUpload);
	RegisterUITexture(L"LoadingVillage", L"../Assets/UI/Textures/LoadingVillage.png", core, resourceUpload);
	RegisterUITexture(L"LoadingCastle", L"../Assets/UI/Textures/LoadingCastle.png", core, resourceUpload);
	RegisterUITexture(L"LoadingCathedral", L"../Assets/UI/Textures/LoadingCathedral.png", core, resourceUpload);
	RegisterUITexture(L"LoadingBarBack", L"../Assets/UI/Textures/LoadingBarBack.png", core, resourceUpload);
	RegisterUITexture(L"LoadingBar", L"../Assets/UI/Textures/LoadingBar.png", core, resourceUpload);
	RegisterUITexture(L"LoadingArrow", L"../Assets/UI/Textures/LoadingArrow.png", core, resourceUpload);
	RegisterUITexture(L"ShortcutHint", L"../Assets/UI/Textures/F1.png", core, resourceUpload);

	RegisterUITexture(L"LocalCharBarsBack", L"../Assets/UI/Textures/LocalCharBarsBack.png", core, resourceUpload);
	RegisterUITexture(L"BarBack", L"../Assets/UI/Textures/BarBack.png", core, resourceUpload);
	RegisterUITexture(L"BossBarBack", L"../Assets/UI/Textures/BossBarBack.png", core, resourceUpload);
	RegisterUITexture(L"HpBar", L"../Assets/UI/Textures/HpBar.png", core, resourceUpload);
	RegisterUITexture(L"HpBar2", L"../Assets/UI/Textures/HpBar2.png", core, resourceUpload);
	RegisterUITexture(L"StaminaBar", L"../Assets/UI/Textures/StaminaBar.png", core, resourceUpload);
	RegisterUITexture(L"Potion", L"../Assets/UI/Textures/Potion.png", core, resourceUpload);

	RegisterUITexture(L"CharBackground", L"../Assets/UI/Textures/CharBackground.png", core, resourceUpload);
	RegisterUITexture(L"CharKnight", L"../Assets/UI/Textures/CharKnight.png", core, resourceUpload);
	RegisterUITexture(L"CharLancer", L"../Assets/UI/Textures/CharLancer.png", core, resourceUpload);
	RegisterUITexture(L"CharPaladin", L"../Assets/UI/Textures/CharPaladin.png", core, resourceUpload);
	RegisterUITexture(L"CharHover", L"../Assets/UI/Textures/CharHover.png", core, resourceUpload);
	RegisterUITexture(L"SelectWindow", L"../Assets/UI/Textures/SelectWindow.png", core, resourceUpload);
	RegisterUITexture(L"OK", L"../Assets/UI/Textures/OK.png", core, resourceUpload);
	RegisterUITexture(L"CANCEL", L"../Assets/UI/Textures/CANCEL.png", core, resourceUpload);

	RegisterUITexture(L"PlazaName", L"../Assets/UI/Textures/PlazaName.png", core, resourceUpload);
	RegisterUITexture(L"VillageName", L"../Assets/UI/Textures/VillageName.png", core, resourceUpload);
	RegisterUITexture(L"CastleName", L"../Assets/UI/Textures/CastleName.png", core, resourceUpload);
	RegisterUITexture(L"FinalName", L"../Assets/UI/Textures/FinalName.png", core, resourceUpload);
	RegisterUITexture(L"StayAlive", L"../Assets/UI/Textures/StayAlive.png", core, resourceUpload);

	RegisterUITexture(L"HappyEnding1", L"../Assets/UI/Textures/HappyEnding1.png", core, resourceUpload);
	RegisterUITexture(L"HappyEnding2", L"../Assets/UI/Textures/HappyEnding2.png", core, resourceUpload);
	RegisterUITexture(L"HappyEndingStory1", L"../Assets/UI/Textures/HappyEndingStory1.png", core, resourceUpload);
	RegisterUITexture(L"HappyEndingStory2", L"../Assets/UI/Textures/HappyEndingStory2.png", core, resourceUpload);

	RegisterUITexture(L"PVPEnding1", L"../Assets/UI/Textures/PVPEnding1.png", core, resourceUpload);
	RegisterUITexture(L"PVPEnding2", L"../Assets/UI/Textures/PVPEnding2.png", core, resourceUpload);
	RegisterUITexture(L"PVPEndingStory1", L"../Assets/UI/Textures/PVPEndingStory1.png", core, resourceUpload);
	RegisterUITexture(L"PVPEndingStory2", L"../Assets/UI/Textures/PVPEndingStory2.png", core, resourceUpload);
	RegisterUITexture(L"PVPEndingStory3", L"../Assets/UI/Textures/PVPEndingStory3.png", core, resourceUpload);
	RegisterUITexture(L"NSkip", L"../Assets/UI/Textures/NSkip.png", core, resourceUpload);

	RegisterUITexture(L"Status", L"../Assets/UI/Textures/Status.png", core, resourceUpload);
	RegisterUITexture(L"StatusBack", L"../Assets/UI/Textures/StatusBack.png", core, resourceUpload);
	RegisterUITexture(L"StatusRibbon",     L"../Assets/UI/Textures/StatusRibbon.png", core, resourceUpload);
	RegisterUITexture(L"StatusArrowLeft",  L"../Assets/UI/Textures/StatusArrowLeft.png", core, resourceUpload);
	RegisterUITexture(L"StatusArrowRight", L"../Assets/UI/Textures/StatusArrowRight.png", core, resourceUpload);
	RegisterUITexture(L"Title1", L"../Assets/UI/Textures/Title1.png", core, resourceUpload);
	RegisterUITexture(L"Title2", L"../Assets/UI/Textures/Title2.png", core, resourceUpload);
	RegisterUITexture(L"Title3", L"../Assets/UI/Textures/Title3.png", core, resourceUpload);
	RegisterUITexture(L"Title4", L"../Assets/UI/Textures/Title4.png", core, resourceUpload);
	RegisterUITexture(L"Title5", L"../Assets/UI/Textures/Title5.png", core, resourceUpload);
	RegisterUITexture(L"Title6", L"../Assets/UI/Textures/Title6.png", core, resourceUpload);
	RegisterUITexture(L"Title7", L"../Assets/UI/Textures/Title7.png", core, resourceUpload);
	RegisterUITexture(L"Title8", L"../Assets/UI/Textures/Title8.png", core, resourceUpload);

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
	RegisterUITexture(L"PartyKnight", L"../Assets/UI/Textures/PartyKnight.png", core, resourceUpload);
	RegisterUITexture(L"PartyLancer", L"../Assets/UI/Textures/PartyLancer.png", core, resourceUpload);
	RegisterUITexture(L"PartyPaladin", L"../Assets/UI/Textures/PartyPaladin.png", core, resourceUpload);

	RegisterUITexture(L"PartyMemBack", L"../Assets/UI/Textures/PartyMemBack.png", core, resourceUpload);
	RegisterUITexture(L"PartyMemKnight", L"../Assets/UI/Textures/PartyMemKnight.png", core, resourceUpload);
	RegisterUITexture(L"PartyMemLancer", L"../Assets/UI/Textures/PartyMemLancer.png", core, resourceUpload);
	RegisterUITexture(L"PartyMemPaladin", L"../Assets/UI/Textures/PartyMemPaladin.png", core, resourceUpload);

	RegisterUITexture(L"EscWindow", L"../Assets/UI/Textures/EscWindow.png", core, resourceUpload);
	RegisterUITexture(L"ESCContinue", L"../Assets/UI/Textures/ESCContinue.png", core, resourceUpload);
	RegisterUITexture(L"ESCSetting", L"../Assets/UI/Textures/ESCSetting.png", core, resourceUpload);
	RegisterUITexture(L"ESCQuit", L"../Assets/UI/Textures/ESCQuit.png", core, resourceUpload);

	RegisterUITexture(L"KeyGuide", L"../Assets/UI/Textures/KeyGuide.png", core, resourceUpload);

	RegisterUITexture(L"MagicCircle", L"../Assets/UI/Textures/MagicCircle.png", core, resourceUpload);
	RegisterUITexture(L"StatueInteractWindow", L"../Assets/UI/Textures/StatueInteractWindow.png", core, resourceUpload);
	RegisterUITexture(L"BeaconInteractWindow", L"../Assets/UI/Textures/BeaconInteractWindow.png", core, resourceUpload);
	RegisterUITexture(L"RespawnWindow", L"../Assets/UI/Textures/RespawnWindow.png", core, resourceUpload);
	RegisterUITexture(L"WITH", L"../Assets/UI/Textures/WITH.png", core, resourceUpload);
	RegisterUITexture(L"ME", L"../Assets/UI/Textures/ME.png", core, resourceUpload);
	RegisterUITexture(L"WE", L"../Assets/UI/Textures/WE.png", core, resourceUpload);

	RegisterUITexture(L"DeathCount", L"../Assets/UI/Textures/DeathCount.png", core, resourceUpload);

	RegisterUITexture(L"VICTORY", L"../Assets/UI/Textures/VICTORY.png", core, resourceUpload);
	RegisterUITexture(L"DEFEAT", L"../Assets/UI/Textures/DEFEAT.png", core, resourceUpload);

	auto uploadFinished = resourceUpload.End(core.GetCmdQueue());
	uploadFinished.wait();

	screenFade = make_unique<ScreenFade>();

	RegisterControllers();
}

void UIManager::Update(float deltaTime)
{
	auto it = controllers.find(currentScene);
	if (it != controllers.end())
		it->second->Update(deltaTime);

	if (screenFade)
		screenFade->Update(deltaTime);
}

void UIManager::Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp)
{
	if (videoPlaying && video)
	{
		video->Update(cmdList);   
		if (video->IsEnded())
			videoPlaying = false; 
	}

	ID3D12DescriptorHeap* heaps[] = { uiSrvHeap->Heap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	spriteBatch->SetViewport(vp);
	spriteBatch->Begin(cmdList);

	if (videoPlaying && video)
	{
		RECT full = { 0, 0, static_cast<LONG>(vp.Width), static_cast<LONG>(vp.Height) };
		if (video->IsReady())
		{
			XMUINT2 texSize{ video->Width(), video->Height() };
			spriteBatch->Draw(uiSrvHeap->GetGpuHandle(videoHeapIndex), texSize, full);
		}
		else if (auto* black = GetUITexture(L"Black"))   
		{
			XMUINT2 texSize = GetTextureSize(black->resource.Get());
			spriteBatch->Draw(uiSrvHeap->GetGpuHandle(black->heapIndex), texSize, full);
		}
		spriteBatch->End();
		graphicsMemory->Commit(cmdQueue);
		return;
	}

	auto it = controllers.find(currentScene);
	if (it != controllers.end())
		it->second->Render(spriteBatch.get());

	if (screenFade && screenFade->IsActive())
	{
		if (auto* black = GetUITexture(L"Black"))
		{
			XMUINT2 texSize = GetTextureSize(black->resource.Get());
			RECT full = { 0, 0, static_cast<LONG>(vp.Width), static_cast<LONG>(vp.Height) };
			XMVECTOR col = XMVectorSet(1.0f, 1.0f, 1.0f, screenFade->GetAlpha());
			spriteBatch->Draw(uiSrvHeap->GetGpuHandle(black->heapIndex), texSize, full, col);
		}
	}

	spriteBatch->End();
	graphicsMemory->Commit(cmdQueue);
}

void UIManager::PlayVideo(const wstring& path)
{
	if (videoPlaying || !coreRef) return;
	if (nextIndex >= MAX_RESOURCE_COUNT - 1) return;

	videoHeapIndex = nextIndex++;
	video = make_unique<VideoPlayer>();
	if (video->Open(coreRef->GetDevice(),
		uiSrvHeap->GetCpuHandle(videoHeapIndex), path))
	{
		videoPlaying = true;
		SOUND_MANAGER->StopBGM(0.0f);   
	}
	else
	{
		video.reset();
	}
}

void UIManager::Release()
{
	if (video) { video->Close(); video.reset(); }
	videoPlaying = false;

	controllers.clear();
	uiTextureMap.clear();
	uiFontMap.clear();
	screenFade.reset();
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
