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

	InitTargetHpBar();
	InitLocalPlayerHUD();
	InitMapNameOverlay();
	InitStatWindow();
	InitMapWindow();
}

void GameSceneUIController::InitTargetHpBar()
{
	constexpr float BARBACK_ASPECT = 39.0f / 785.0f;
	constexpr float HPBAR_WIDTH_RATIO = 692.0f / 785.0f;
	constexpr float HPBAR_HEIGHT_RATIO = 18.0f / 39.0f;
	constexpr float HPBAR_OFFSET_X = 49.0f / 785.0f;
	constexpr float HPBAR_OFFSET_Y = 11.0f / 39.0f;

	float backWidth = 300.0f;
	float backHeight = backWidth * BARBACK_ASPECT;

	float backPosX = WinSize.x * 0.5f - backWidth * 0.5f;
	float backPosY = WinSize.y * 0.5f - backHeight * 0.5f;

	charHPBarBack = make_shared<ImageUI>(L"BarBack", ImageUIState::Visible);
	charHPBarBack->Init(uiManager);
	charHPBarBack->SetPosition(backPosX, backPosY);
	charHPBarBack->SetHoriLength(backWidth);
	charHPBarBack->SetVertLength(backHeight);
	widgets.push_back(charHPBarBack);

	float hpBarWidth = backWidth * HPBAR_WIDTH_RATIO;
	float hpBarHeight = backHeight * HPBAR_HEIGHT_RATIO;
	float hpBarPosX = backPosX + backWidth * HPBAR_OFFSET_X;
	float hpBarPosY = backPosY + backHeight * HPBAR_OFFSET_Y;

	charHPBar = make_shared<ImageUI>(L"HpBar2", ImageUIState::Visible);
	charHPBar->Init(uiManager);
	charHPBar->SetPosition(hpBarPosX, hpBarPosY);
	charHPBar->SetHoriLength(hpBarWidth);
	charHPBar->SetVertLength(hpBarHeight);
	widgets.push_back(charHPBar);
}

void GameSceneUIController::InitLocalPlayerHUD()
{
	localCharBarsBack = make_shared<ImageUI>(L"LocalCharBarsBack", ImageUIState::Visible);
	localCharBarsBack->Init(uiManager);
	localCharBarsBack->SetPosition(WinSize.x * 0.02f, WinSize.y * 0.03f);
	localCharBarsBack->SetHoriLength(WinSize.y * 0.512);
	localCharBarsBack->SetVertLength(WinSize.y * 0.1f);
	widgets.push_back(localCharBarsBack);

	localCharHpBar = make_shared<ImageUI>(L"HpBar", ImageUIState::Visible);
	localCharHpBar->Init(uiManager);
	localCharHpBar->SetPosition(WinSize.x * 0.0758f, WinSize.y * 0.0621f);
	localCharHpBar->SetHoriLength(WinSize.y * 0.3457);
	localCharHpBar->SetVertLength(WinSize.y * 0.0095);
	widgets.push_back(localCharHpBar);

	localCharStaminaBar = make_shared<ImageUI>(L"StaminaBar", ImageUIState::Visible);
	localCharStaminaBar->Init(uiManager);
	localCharStaminaBar->SetPosition(WinSize.x * 0.0767f, WinSize.y * 0.087499f);
	localCharStaminaBar->SetHoriLength(WinSize.y * 0.2566);
	localCharStaminaBar->SetVertLength(WinSize.y * 0.00626);
	widgets.push_back(localCharStaminaBar);
}

void GameSceneUIController::InitMapNameOverlay()
{
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
	widgets.push_back(mapNameImage);
}

void GameSceneUIController::InitStatWindow()
{
	statusImage = make_shared<ImageUI>(L"Status", ImageUIState::Hidden);
	statusImage->Init(uiManager);
	statusImage->SetHoriLength(WinSize.x);
	statusImage->SetVertLength(WinSize.y);
	widgets.push_back(statusImage);

	tempStatusText = make_shared<TextUI>(L"Texture", L"MalgunGothic");
	tempStatusText->Init(uiManager);
	tempStatusText->SetPosition(0.0f, 0.0f);
	tempStatusText->SetText(L"TempText");
	widgets.push_back(tempStatusText);
}

void GameSceneUIController::InitMapWindow()
{
	wstring texName;
	switch (sceneType)
	{
	case SceneType::Plaza:   texName = L"PlazaMap";   break;
	case SceneType::Village: texName = L"VillageMap";  break;
	case SceneType::Castle:  texName = L"CastleMap";   break;
	}

	mapBackImage = make_shared<ImageUI>(L"Black", ImageUIState::Hidden);
	mapBackImage->Init(uiManager);
	mapBackImage->SetPosition(0.0f, 0.0f);
	mapBackImage->SetHoriLength(WinSize.x);
	mapBackImage->SetVertLength(WinSize.y);
	mapBackImage->SetTintAlpha(0.98f);
	widgets.push_back(mapBackImage);

	mapImage = make_shared<ImageUI>(texName, ImageUIState::Hidden);
	mapImage->Init(uiManager);
	mapImage->SetPosition(WinSize.x * 0.2031f, 0.0f);
	mapImage->SetHoriLength(WinSize.y * (1140.0f/1080.0f));
	mapImage->SetVertLength(WinSize.y);
	widgets.push_back(mapImage);
}

void GameSceneUIController::Update(float deltaTime)
{
	for (auto& w : widgets) w->Update(deltaTime);

	if (INPUT.GetKeyDown('K'))
	{
		ImageUIState next = (statusImage->GetState() == ImageUIState::Hidden)
			? ImageUIState::Visible : ImageUIState::Hidden;

		statusImage->ChangeState(next);
	}

	if (INPUT.GetKeyDown('M'))
	{
		if (sceneType >= SceneType::Plaza && sceneType <= SceneType::Castle)
		{
			ImageUIState next = (mapImage->GetState() == ImageUIState::Hidden)
				? ImageUIState::Visible : ImageUIState::Hidden;

			mapBackImage->ChangeState(next);
			mapImage->ChangeState(next);
		}
	}
}

void GameSceneUIController::Render(SpriteBatch* batch)
{
	for (auto& w : widgets) w->Render(batch);
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
	return statusImage ? (statusImage->GetState() == ImageUIState::Visible) : false;
}
