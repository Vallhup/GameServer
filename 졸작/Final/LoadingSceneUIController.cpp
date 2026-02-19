#include "pch.h"
#include "LoadingSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Engine.h"
#include "Input.h"

void LoadingSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	mainImage = make_shared<ImageUI>(L"LoadingPage", ImageUIState::Visible);
	mainImage->Init(uiManager);
	mainImage->SetHoriLength(WinSize.x);
	mainImage->SetVertLength(WinSize.y);

	loadBarBackImage = make_shared<ImageUI>(L"LoadingBarBack", ImageUIState::Visible);
	loadBarBackImage->Init(uiManager);
	loadBarBackImage->SetPosition((WinSize.x * 0.4f) / 2.f, WinSize.y * 0.7f);
	loadBarBackImage->SetHoriLength(WinSize.x * 0.6f);
	loadBarBackImage->SetVertLength(WinSize.y * 0.16f);

	loadBar = make_shared<ImageUI>(L"LoadingBar", ImageUIState::Visible);
	loadBar->Init(uiManager);
	loadBar->SetPosition((WinSize.x * 0.5f) / 2.f, WinSize.y * 0.766f);
	loadBar->SetHoriLength(0);  // 처음에는 0
	loadBar->SetVertLength(WinSize.y * 0.03f);

	loadBarMaxWidth = WinSize.x * 0.5f;

	loadArrow = make_shared<ImageUI>(L"LoadingArrow", ImageUIState::Visible);
	loadArrow->Init(uiManager);
	loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f, WinSize.y * 0.731f);
	loadArrow->SetHoriLength(WinSize.y * 0.1f);
	loadArrow->SetVertLength(WinSize.y * 0.1f);
}

void LoadingSceneUIController::Update(float deltaTime)
{
	if (mainImage) mainImage->Update(deltaTime);
	if (loadBarBackImage) loadBarBackImage->Update(deltaTime);
	if (loadBar) loadBar->Update(deltaTime);
	if (loadArrow) loadArrow->Update(deltaTime);

	if (loadProgress >= 1.0f && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SCENE_MANAGER->RequestSceneChange(SceneType::Select);
	}
}

void LoadingSceneUIController::Render(SpriteBatch* batch)
{
	if (mainImage) mainImage->Render(batch);
	if (loadBarBackImage) loadBarBackImage->Render(batch);
	if (loadBar) loadBar->Render(batch);
	if (loadArrow) loadArrow->Render(batch);
}

void LoadingSceneUIController::SetProgress(float progress)
{
	loadProgress = progress;

	if (loadBar) {
		loadBar->SetHoriLength(loadBarMaxWidth * progress);
	}

	if (loadArrow) {
		loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f + loadBarMaxWidth * progress, WinSize.y * 0.731f);
	}
}
