#include "pch.h"
#include "GameSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Input.h"

void GameSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	// Status 패널
	statusImage = make_shared<ImageUI>(L"Status", ImageUIState::Hidden);
	statusImage->Init(uiManager);
	statusImage->SetPosition(WinSize.x * 0.5f, WinSize.y * 0.25f);
	statusImage->SetHoriLength(WinSize.x * 0.35f);
	statusImage->SetVertLength(WinSize.y * 0.5f);
	statusImage->SetFadeDuration(1.0f);

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
	if (statusImage) statusImage->Update(deltaTime);
	if (localCharBarsBack) localCharBarsBack->Update(deltaTime);
	if (localCharHpBar) localCharHpBar->Update(deltaTime);
	if (localCharStaminaBar) localCharStaminaBar->Update(deltaTime);

	if (INPUT.GetKeyDown('K'))
	{
		if (statusImage->GetState() == ImageUIState::Hidden)
			statusImage->ChangeState(ImageUIState::FadingIn);
		else if (statusImage->GetState() == ImageUIState::FadingIn || statusImage->GetState() == ImageUIState::Visible)
			statusImage->ChangeState(ImageUIState::Hidden);
	}
}

void GameSceneUIController::Render(SpriteBatch* batch)
{
	if (statusImage) statusImage->Render(batch);
	if (localCharBarsBack) localCharBarsBack->Render(batch);
	if (localCharHpBar) localCharHpBar->Render(batch);
	if (localCharStaminaBar) localCharStaminaBar->Render(batch);
}
