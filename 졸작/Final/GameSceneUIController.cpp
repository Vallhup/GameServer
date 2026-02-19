#include "pch.h"
#include "GameSceneUIController.h"
#include "PanelUI.h"
#include "ImageUI.h"
#include "UIManager.h"

void GameSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	// Status 패널
	statusPanel = make_shared<PanelUI>(L"Status");
	statusPanel->Init(uiManager);
	statusPanel->SetPosition(300.f, 150.f);
	statusPanel->SetScale(0.5f);

	localCharBarsBack = make_shared<ImageUI>(L"LocalCharBarsBack", ImageUIState::Visible);
	localCharBarsBack->Init(uiManager);
	localCharBarsBack->SetPosition(WinSize.x * 0.02f, WinSize.y * 0.03f);
	localCharBarsBack->SetHoriLength(WinSize.y * 0.512);
	localCharBarsBack->SetVertLength(WinSize.y * 0.1f);

	localCharHpBar = make_shared<ImageUI>(L"HpBar", ImageUIState::Visible);
	localCharHpBar->Init(uiManager);
	localCharHpBar->SetPosition(WinSize.x * 0.0758f, WinSize.y * 0.0621f);
	localCharHpBar->SetHoriLength(WinSize.y * 0.3457);
	localCharHpBar->SetVertLength(WinSize.y * 0.0095);

	localCharStaminaBar = make_shared<ImageUI>(L"StaminaBar", ImageUIState::Visible);
	localCharStaminaBar->Init(uiManager);
	localCharStaminaBar->SetPosition(WinSize.x * 0.0767f, WinSize.y * 0.087499f);
	localCharStaminaBar->SetHoriLength(WinSize.y * 0.2566);
	localCharStaminaBar->SetVertLength(WinSize.y * 0.00626);
}

void GameSceneUIController::Update(float deltaTime)
{
	if (statusPanel) statusPanel->Update(deltaTime);
	if (localCharBarsBack) localCharBarsBack->Update(deltaTime);
	if (localCharHpBar) localCharHpBar->Update(deltaTime);
	if (localCharStaminaBar) localCharStaminaBar->Update(deltaTime);
}

void GameSceneUIController::Render(SpriteBatch* batch)
{
	if (statusPanel && statusPanel->IsVisible()) statusPanel->Render(batch);
	if (localCharBarsBack) localCharBarsBack->Render(batch);
	if (localCharHpBar) localCharHpBar->Render(batch);
	if (localCharStaminaBar) localCharStaminaBar->Render(batch);
}
