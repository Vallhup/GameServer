#include "pch.h"
#include "GameSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Input.h"
#include "TextUI.h"

GameSceneUIController::GameSceneUIController(SceneType type) : sceneType(type) {}

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

	// 맵 이름 이미지
	wstring texName;
	switch (sceneType)
	{
	case SceneType::Plaza:   texName = L"PlazaName";   break;
	case SceneType::Village: texName = L"VillageName";  break;
	case SceneType::Castle:  texName = L"CastleName";   break;
	case SceneType::Final:   texName = L"FinalName";    break;
	}
	mapNameImage = make_shared<ImageUI>(texName, ImageUIState::Hidden);
	mapNameImage->Init(uiManager);
	mapNameImage->SetPosition(WinSize.x * 0.3f, WinSize.y * 0.1f);
	mapNameImage->SetHoriLength(WinSize.x * 0.4f);
	mapNameImage->SetVertLength(WinSize.y * 0.1f);

	statBackground = make_shared<ImageUI>(L"StatBackground", ImageUIState::Hidden);
	statBackground->Init(uiManager);
	statBackground->SetHoriLength(WinSize.x);
	statBackground->SetVertLength(WinSize.y);

	// 비율 상수 (BarBack 785x39, HpBar 692x18, 오프셋 47,11)
	constexpr float BARBACK_ASPECT    = 39.0f / 785.0f;    // 0.0497
	constexpr float HPBAR_WIDTH_RATIO  = 692.0f / 785.0f;  // 0.8815
	constexpr float HPBAR_HEIGHT_RATIO = 18.0f / 39.0f;    // 0.4615
	constexpr float HPBAR_OFFSET_X     = 49.0f / 785.0f;   // 0.0599
	constexpr float HPBAR_OFFSET_Y     = 11.0f / 39.0f;    // 0.2821

	// BarBack 기준 크기 설정 (가로 300px 테스트)
	float backWidth = 300.0f;
	float backHeight = backWidth * BARBACK_ASPECT;

	// 화면 중앙 배치 (좌상단 기준이므로 절반 빼기)
	float backPosX = WinSize.x * 0.5f - backWidth * 0.5f;
	float backPosY = WinSize.y * 0.5f - backHeight * 0.5f;

	charHPBarBack = make_shared<ImageUI>(L"BarBack", ImageUIState::Visible);
	charHPBarBack->Init(uiManager);
	charHPBarBack->SetPosition(backPosX, backPosY);
	charHPBarBack->SetHoriLength(backWidth);
	charHPBarBack->SetVertLength(backHeight);

	// HpBar: BarBack 기준 비율로 계산
	float hpBarWidth = backWidth * HPBAR_WIDTH_RATIO;
	float hpBarHeight = backHeight * HPBAR_HEIGHT_RATIO;
	float hpBarPosX = backPosX + backWidth * HPBAR_OFFSET_X;
	float hpBarPosY = backPosY + backHeight * HPBAR_OFFSET_Y;

	charHPBar = make_shared<ImageUI>(L"HpBar2", ImageUIState::Visible);
	charHPBar->Init(uiManager);
	charHPBar->SetPosition(hpBarPosX, hpBarPosY);
	charHPBar->SetHoriLength(hpBarWidth);
	charHPBar->SetVertLength(hpBarHeight);
}

void GameSceneUIController::Update(float deltaTime)
{
	if (statusImage) statusImage->Update(deltaTime);
	if (localCharBarsBack) localCharBarsBack->Update(deltaTime);
	if (localCharHpBar) localCharHpBar->Update(deltaTime);
	if (localCharStaminaBar) localCharStaminaBar->Update(deltaTime);

	if (charHPBarBack) charHPBarBack->Update(deltaTime);
	if (charHPBar) charHPBar->Update(deltaTime);
	if (mapNameImage) mapNameImage->Update(deltaTime);

	if (statBackground) statBackground->Update(deltaTime);

	if (INPUT.GetKeyDown('K'))
	{
		if (statusImage->GetState() == ImageUIState::Hidden)
			statusImage->ChangeState(ImageUIState::FadingIn);
		else if (statusImage->GetState() == ImageUIState::FadingIn || statusImage->GetState() == ImageUIState::Visible)
			statusImage->ChangeState(ImageUIState::Hidden);
	}

	if (INPUT.GetKeyDown(VK_TAB))
	{
		if (statBackground->GetState() == ImageUIState::Hidden)
			statBackground->ChangeState(ImageUIState::Visible);
		else
			statBackground->ChangeState(ImageUIState::Hidden);
	}
}

void GameSceneUIController::Render(SpriteBatch* batch)
{
	if (statusImage) statusImage->Render(batch);
	if (localCharBarsBack) localCharBarsBack->Render(batch);
	if (localCharHpBar) localCharHpBar->Render(batch);
	if (localCharStaminaBar) localCharStaminaBar->Render(batch);
	if (tempStatusText) tempStatusText->Render(batch);
	if (charHPBarBack) charHPBarBack->Render(batch);
	if (charHPBar) charHPBar->Render(batch);
	if (mapNameImage) mapNameImage->Render(batch);
	if (statBackground) statBackground->Render(batch);
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

void GameSceneUIController::ShowMapName()
{
	if (mapNameImage)
		mapNameImage->ChangeState(ImageUIState::PulseOnce);
}

bool GameSceneUIController::IsStatWindowOn() const
{
	return statusImage ? 
		(statusImage->GetState() == ImageUIState::FadingIn || statusImage->GetState() == ImageUIState::Visible) : false;
}
