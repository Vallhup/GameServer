#include "pch.h"
#include "GameSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Input.h"
#include "TextUI.h"

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

	tempStatusText = make_shared<TextUI>(L"Texture", L"MalgunGothic");
	tempStatusText->Init(uiManager);
	tempStatusText->SetPosition(0.0f, 0.0f);
	tempStatusText->SetText(L"TempText");
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
	if (tempStatusText) tempStatusText->Render(batch);
}

void GameSceneUIController::HandleStatBarChange(int curHp, int maxHp, int curStamina, int maxStamina)
{
	const float maxHpLength = WinSize.y * 0.3457;
	const float hpPercent = (float)curHp / maxHp;
	localCharHpBar->SetHoriLength(maxHpLength * hpPercent);

	const float maxStaminaLength = WinSize.y * 0.2566;
	const float staminaPercent = (float)curStamina / maxStamina;
	localCharStaminaBar->SetHoriLength(maxStaminaLength * staminaPercent);
}

void GameSceneUIController::HandleStatImageChange(int curHp, int maxHp, int curStamina, int maxStamina, int power, double aSpeed, int defense, double mSpeed)
{
	// TODO : StatusText 변경
	std::wstring text =
		L"curHp: " + to_wstring(curHp) + L"\n" +
		L"maxHp: " + to_wstring(maxHp) + L"\n" +
		L"curStamina: " + to_wstring(curStamina) + L"\n" +
		L"maxStamina: " + to_wstring(maxStamina) + L"\n" +
		L"power: " + to_wstring(power) + L"\n" +
		L"aSpeed: " + to_wstring(aSpeed) + L"\n" +
		L"defense" + to_wstring(defense) + L"\n" +
		L"mSpeed: " + to_wstring(mSpeed) + L"\n";

	if (tempStatusText)
		tempStatusText->SetText(text);
}

bool GameSceneUIController::IsStatWindowOn() const
{
	return statusImage ? 
		(statusImage->GetState() == ImageUIState::FadingIn || statusImage->GetState() == ImageUIState::Visible) : false;
}
