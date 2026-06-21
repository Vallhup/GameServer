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

	InitBackground();
	InitLoadBar();
	InitPressAnyButton();
	InitShortcutHint();
}

void LoadingSceneUIController::InitBackground()
{
	mainImage = make_shared<ImageUI>(uiManager, L"LoadingPage", ImageUIState::Visible);
	mainImage->SetHoriLength(WinSize.x);
	mainImage->SetVertLength(WinSize.y);
	widgets.push_back(mainImage);
}

void LoadingSceneUIController::InitLoadBar()
{
	loadBarBack = make_shared<ImageUI>(uiManager, L"LoadingBarBack", ImageUIState::Visible);
	loadBarBack->SetPosition((WinSize.x * 0.4f) / 2.f, WinSize.y * 0.7f);
	loadBarBack->SetHoriLength(WinSize.x * 0.6f);
	loadBarBack->SetVertLength(WinSize.y * 0.16f);
	widgets.push_back(loadBarBack);

	loadBar = make_shared<ImageUI>(uiManager, L"LoadingBar", ImageUIState::Visible);
	loadBar->SetPosition((WinSize.x * 0.5f) / 2.f, WinSize.y * 0.765f);
	loadBar->SetHoriLength(0);  // 처음에는 0
	loadBar->SetVertLength(WinSize.y * 0.03f);
	widgets.push_back(loadBar);

	loadBarMaxWidth = WinSize.x * 0.5f;

	loadArrow = make_shared<ImageUI>(uiManager, L"LoadingArrow", ImageUIState::Visible);
	loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f, WinSize.y * 0.73f);
	loadArrow->SetHoriLength(WinSize.y * 0.1f);
	loadArrow->SetVertLength(WinSize.y * 0.1f);
	widgets.push_back(loadArrow);
}

void LoadingSceneUIController::InitPressAnyButton()
{
	pab = make_shared<ImageUI>(uiManager, L"PressAnyButton", ImageUIState::Hidden);
	pab->SetPosition(WinSize.x * 0.4f, WinSize.y * 0.85f);
	pab->SetHoriLength(WinSize.x * 0.2f);
	pab->SetVertLength(WinSize.y * 0.04f);
	widgets.push_back(pab);
}

void LoadingSceneUIController::InitShortcutHint()
{
	shortcutHint = make_shared<ImageUI>(uiManager, L"ShortcutHint", ImageUIState::Hidden);
	const float texAspect = 3471.0f / 257.0f;
	const float hintWidth = WinSize.x * 0.4f;
	const float hintHeight = hintWidth / texAspect;
	shortcutHint->SetPosition((WinSize.x - hintWidth) * 0.5f, WinSize.y * 0.68f);
	shortcutHint->SetHoriLength(hintWidth);
	shortcutHint->SetVertLength(hintHeight);
	widgets.push_back(shortcutHint);
}

void LoadingSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	if (loadProgress >= 1.0f)
	{
		if (pab->GetState() == ImageUIState::Hidden)
			pab->ChangeState(ImageUIState::Pulsing);

		if (INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SCENE_MANAGER->RequestSceneChange(targetScene);
		}
	}
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

	wstring texName = L"LoadingPage";
	switch (targetScene)
	{
	case SceneType::Plaza:   texName = L"LoadingPlaza"; break;
	case SceneType::Village: texName = L"LoadingVillage";  break;
	case SceneType::Castle:  texName = L"LoadingCastle";  break;
	case SceneType::Final:   texName = L"LoadingCathedral";  break;
	}

	if (mainImage) mainImage->SetTexture(texName);

	if (shortcutHint)
		shortcutHint->ChangeState(targetScene == SceneType::Plaza ? ImageUIState::Visible : ImageUIState::Hidden);
}

void LoadingSceneUIController::Reset()
{
	loadProgress = 0.0f;
	if (loadBar) loadBar->SetHoriLength(0);
	if (loadArrow) loadArrow->SetPosition((WinSize.x * 0.446f) / 2.f, WinSize.y * 0.73f);
	if (pab) pab->ChangeState(ImageUIState::Hidden);
}
