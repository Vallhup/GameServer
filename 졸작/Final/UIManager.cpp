#include "pch.h"
#include "UIManager.h"
#include <WICTextureLoader.h>
#include <DirectXHelpers.h>
#include "Engine.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "MainCharacter.h"
#include "PanelUI.h"
#include "TextUI.h"
#include "ImageUI.h"

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

	RegisterFont(L"MalgunGothic", L"../Assets/UI/Fonts/MalgunGothic.spritefont", core, resourceUpload);	// ∏º¿∫∞ÌµÒ
	RegisterUITexture(L"MainPage", L"../Assets/UI/Textures/MainPage.png", core, resourceUpload);		// ∏ﬁ¿Œ»≠∏È
	RegisterUITexture(L"PAB", L"../Assets/UI/Textures/PAB.png", core, resourceUpload);					// PRESS ANY BUTTON
	RegisterUITexture(L"LOGIN", L"../Assets/UI/Textures/LOGIN.png", core, resourceUpload);
	RegisterUITexture(L"EXIT", L"../Assets/UI/Textures/EXIT.png", core, resourceUpload);
	RegisterUITexture(L"Status", L"../Assets/UI/Textures/Status.png", core, resourceUpload);			// Ω∫≈»√¢

	RegisterComponents();

	auto uploadFinished = resourceUpload.End(core.GetCmdQueue());
	uploadFinished.wait();
}

void UIManager::Update(float deltaTime)
{
	// Temporarily
	if (INPUT.GetKeyDown('K'))
	{
		auto status = GetUIComponent<PanelUI>(L"Status");
		if (status)
			status->Toggle();
	}

	for (auto& comp : sceneUIMap[currentScene])
	{
		if (comp->IsVisible())
			comp->Update(deltaTime);
	}
}

void UIManager::Render(ID3D12GraphicsCommandList* cmdList, ID3D12CommandQueue* cmdQueue, const D3D12_VIEWPORT& vp)
{
	ID3D12DescriptorHeap* heaps[] = { uiSrvHeap->Heap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	spriteBatch->SetViewport(vp);
	spriteBatch->Begin(cmdList);

	//static bool status = false;

	//if (INPUT.GetKeyDown('K'))
	//{
	//	status = !status;
	//	if (status)
	//	{
	//		auto& statusTex = uiTextureMap[L"Status"];
	//		statusTex.fadeElapsed = 0.0f;
	//		statusTex.fadeDuration = 2.0f;
	//		statusTex.fadeAlpha = 0.0f;
	//		statusTex.fading = true;
	//	}
	//}

	//if (status)
	//{
	//	auto& statusTex = uiTextureMap[L"Status"];

	//	XMUINT2 texSize = GetTextureSize(statusTex.resource.Get());
	//	RECT destRect = { 0, 0, static_cast<LONG>(texSize.x * 0.5f), static_cast<LONG>(texSize.y * 0.5f) };
	//	XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, statusTex.fadeAlpha);
	//	spriteBatch->Draw(uiSrvHeap->GetGpuHandle(statusTex.heapIndex), texSize, destRect, color);
	//}

	//auto& font = uiFontMap[L"MalgunGothic"].font;
	////XMVECTOR textSize = font->MeasureString(L"Hello, I'm JeongHo Lee");
	////XMFLOAT2 origin(XMVectorGetX(textSize) / 2.f, XMVectorGetY(textSize) / 2.f);
	////XMFLOAT2 pos(vp.Width / 2.f, vp.Height / 2.f);
	////font->DrawString(spriteBatch.get(), L"Hello, I'm JeongHo Lee", pos, Colors::White, 0.f, origin);

	//if (SCENE_MANAGER->GetCurrentSceneType() == SceneType::MainGame)
	//{
	//	auto player = static_cast<GameScene*>(SCENE_MANAGER->GetCurrentScene())->GetMyPlayer();
	//	auto cam = SCENE_MANAGER->GetCurrentScene()->GetCamera();
	//	auto camPos = cam->GetPosition();

	//	auto playerPos = player->GetComponent<Transform>()->GetPosition();
	//	playerPos.y += 2.2f;

	//	XMMATRIX view = cam->GetViewMatrix();
	//	XMMATRIX proj = cam->GetProjectionMatrix();

	//	XMVECTOR screenPos = XMVector3Project(XMLoadFloat3(&playerPos),
	//		vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height,
	//		vp.MinDepth, vp.MaxDepth, proj, view, XMMatrixIdentity());

	//	XMFLOAT3 screen;
	//	XMStoreFloat3(&screen, screenPos);

	//	XMVECTOR textSize2 = font->MeasureString(L"Health Bar");
	//	XMFLOAT2 origin2(XMVectorGetX(textSize2) / 2.f, XMVectorGetY(textSize2) / 2.f);
	//	XMFLOAT2 pos2(screen.x, screen.y);
	//	float dist = XMVectorGetX(XMVector3Length(XMLoadFloat3(&playerPos) - XMLoadFloat3(&camPos)));
	//	float scale = 4.5f / dist;
	//	font->DrawString(spriteBatch.get(), L"Health Bar", pos2, Colors::White, 0.f, origin2, scale);
	//}

	for (auto& comp : sceneUIMap[currentScene])
	{
		if (comp->IsVisible())
			comp->Render(spriteBatch.get());
	}

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

void UIManager::AddUIComponent(shared_ptr<UIComponent> comp)
{
	SceneType scene = comp->GetOwnerSceneType();
	sceneUIMap[scene].push_back(comp);
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

void UIManager::RegisterComponents()
{
#pragma region TestScene UI
	auto mainImage = make_shared<ImageUI>(L"MainPage");
	mainImage->Init(this, SceneType::Start);
	mainImage->SetHoriLength(WinSize.x);
	mainImage->SetVertLength(WinSize.y);
	mainImage->SetVisible(true);
	AddUIComponent(mainImage);

	auto pabImage = make_shared<ImageUI>(L"PAB");
	pabImage->Init(this, SceneType::Start);
	pabImage->SetPosition((WinSize.x * 0.727f) / 2.f, WinSize.y * 0.7f);
	pabImage->SetHoriLength(WinSize.x * 0.273f);
	pabImage->SetVertLength(WinSize.y * 0.083f);
	AddUIComponent(pabImage);

	auto loginImage = make_shared<ImageUI>(L"LOGIN");
	loginImage->Init(this, SceneType::Start);
	loginImage->SetPosition(WinSize.x * 0.3215f, WinSize.y * 0.7f);
	loginImage->SetHoriLength(WinSize.x * 0.117f);
	loginImage->SetVertLength(WinSize.y * 0.1f);
	AddUIComponent(loginImage);

	auto exitImage = make_shared<ImageUI>(L"EXIT");
	exitImage->Init(this, SceneType::Start);
	exitImage->SetPosition(WinSize.x * 0.5615f, WinSize.y * 0.7f);
	exitImage->SetHoriLength(WinSize.x * 0.117f);
	exitImage->SetVertLength(WinSize.y * 0.1f);
	AddUIComponent(exitImage);

	mainImage->SetOnFadeComplete([pabImage]() {
		pabImage->SetVisible(true);
		pabImage->SetPulsing(true); });

	pabImage->SetOnPulsing([pabImage, loginImage, exitImage]() {
		pabImage->SetVisible(false);
		pabImage->SetPulsing(false);

		loginImage->SetVisible(true);
		loginImage->SetPulsing(true);

		exitImage->SetVisible(true);
		exitImage->SetPulsing(true); });
#pragma endregion

#pragma region GameScene UI
	auto statusPanel = make_shared<PanelUI>(L"Status");
	statusPanel->Init(this, SceneType::MainGame);
	statusPanel->SetPosition(300.f, 150.f);
	statusPanel->SetScale(0.5f);
	AddUIComponent(statusPanel);

	auto testText = make_shared<TextUI>(L"TestText", L"MalgunGothic");
	testText->Init(this, SceneType::MainGame);
	testText->SetText(L"Hello World");
	testText->SetPosition(100.f, 100.f);
	testText->SetVisible(true);
	AddUIComponent(testText);
#pragma endregion
}
