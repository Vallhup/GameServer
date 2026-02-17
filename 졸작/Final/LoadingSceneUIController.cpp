#include "pch.h"
#include "LoadingSceneUIController.h"
#include "ImageUI.h"
#include "TextUI.h"
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

	progressText = make_shared<TextUI>(L"ProgressText", L"MalgunGothic");
	progressText->Init(uiManager);
	progressText->SetPosition(WinSize.x * 0.8f, WinSize.y * 0.8f);
	progressText->SetText(L"0%");
	progressText->SetVisible(true);
}

void LoadingSceneUIController::Update(float deltaTime)
{
	if (mainImage) mainImage->Update(deltaTime);
	if (loadBarBackImage) loadBarBackImage->Update(deltaTime);
	if (progressText) progressText->Update(deltaTime);

	if (loadBarBackImage->GetState() == ImageUIState::Visible &&
		INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SCENE_MANAGER->RequestSceneChange(SceneType::Select);
	}
}

void LoadingSceneUIController::Render(SpriteBatch* batch)
{
	if (mainImage) mainImage->Render(batch);
	if (loadBarBackImage) loadBarBackImage->Render(batch);
	if (progressText) progressText->Render(batch);
}

void LoadingSceneUIController::SetProgress(float progress)
{
	loadProgress = progress;
	if (progressText) {
		int percent = (int)(progress * 100);
		progressText->SetText(to_wstring(percent) + L"%");
	}
}
