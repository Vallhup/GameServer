#include "pch.h"
#include "LoadingSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Engine.h"
#include "Input.h"
#include "SceneManager.h"

void LoadingSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	mainImage = make_shared<ImageUI>(L"LoadingPage", ImageUIState::Visible);
	mainImage->Init(uiManager);
	mainImage->SetHoriLength(WinSize.x);
	mainImage->SetVertLength(WinSize.y);

	loadBarBack = make_shared<ImageUI>(L"LoadingBarBack", ImageUIState::Visible);
	loadBarBack->Init(uiManager);
	loadBarBack->SetPosition((WinSize.x * 0.4f) / 2.f, WinSize.y * 0.7f);
	loadBarBack->SetHoriLength(WinSize.x * 0.6f);
	loadBarBack->SetVertLength(WinSize.y * 0.16f);

	loadBar = make_shared<ImageUI>(L"LoadingBar", ImageUIState::Visible);
	loadBar->Init(uiManager);
	loadBar->SetPosition((WinSize.x * 0.5f) / 2.f, WinSize.y * 0.765f);
	loadBar->SetHoriLength(0);  // 처음에는 0
	loadBar->SetVertLength(WinSize.y * 0.03f);

	loadBarMaxWidth = WinSize.x * 0.5f;

	loadArrow = make_shared<ImageUI>(L"LoadingArrow", ImageUIState::Visible);
	loadArrow->Init(uiManager);
	loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f, WinSize.y * 0.73f);
	loadArrow->SetHoriLength(WinSize.y * 0.1f);
	loadArrow->SetVertLength(WinSize.y * 0.1f);

	pab = make_shared<ImageUI>(L"PressAnyButton", ImageUIState::Hidden);
	pab->Init(uiManager);
	pab->SetPosition(WinSize.x * 0.4f, WinSize.y * 0.85f);
	pab->SetHoriLength(WinSize.x * 0.2f);
	pab->SetVertLength(WinSize.y * 0.04f);
}

void LoadingSceneUIController::Update(float deltaTime)
{
	if (mainImage) mainImage->Update(deltaTime);
	if (loadBarBack) loadBarBack->Update(deltaTime);
	if (loadBar) loadBar->Update(deltaTime);
	if (loadArrow) loadArrow->Update(deltaTime);
	if (pab) pab->Update(deltaTime);

	if (loadProgress >= 1.0f)
	{
		if (pab->GetState() == ImageUIState::Hidden)
			pab->ChangeState(ImageUIState::Pulsing);

		if (INPUT.GetMouseButtonDown(MouseButton::LEFT))
			SCENE_MANAGER->RequestSceneChange(targetScene);
	}
}

void LoadingSceneUIController::Render(SpriteBatch* batch)
{
	if (mainImage) mainImage->Render(batch);
	if (loadBarBack) loadBarBack->Render(batch);
	if (loadBar) loadBar->Render(batch);
	if (loadArrow) loadArrow->Render(batch);
	if (pab) pab->Render(batch);
}

void LoadingSceneUIController::SetProgress(float progress)
{
	loadProgress = progress;

	if (loadBar) {
		loadBar->SetHoriLength(loadBarMaxWidth * progress);
	}

	if (loadArrow) {
		loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f + loadBarMaxWidth * progress, WinSize.y * 0.73f);
	}
}

void LoadingSceneUIController::SetTargetScene(SceneType type)
{
	targetScene = type;
}

void LoadingSceneUIController::Reset()
{
	loadProgress = 0.0f;
	if (loadBar) loadBar->SetHoriLength(0);
	if (loadArrow) loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f, WinSize.y * 0.73f);
	if (pab) pab->ChangeState(ImageUIState::Hidden);
}
