#include "pch.h"
#include "GameSceneUIController.h"
#include "PanelUI.h"
#include "TextUI.h"
#include "UIManager.h"

void GameSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	// Status 패널
	statusPanel = make_shared<PanelUI>(L"Status");
	statusPanel->Init(uiManager, SceneType::MainGame);
	statusPanel->SetPosition(300.f, 150.f);
	statusPanel->SetScale(0.5f);

	// 테스트 텍스트
	testText = make_shared<TextUI>(L"TestText", L"MalgunGothic");
	testText->Init(uiManager, SceneType::MainGame);
	testText->SetText(L"Hello World");
	testText->SetPosition(100.f, 100.f);
	testText->SetVisible(true);
}

void GameSceneUIController::Update(float deltaTime)
{
	if (statusPanel) statusPanel->Update(deltaTime);
	if (testText) testText->Update(deltaTime);
}

void GameSceneUIController::Render(SpriteBatch* batch)
{
	if (statusPanel && statusPanel->IsVisible()) statusPanel->Render(batch);
	if (testText && testText->IsVisible()) testText->Render(batch);
}
