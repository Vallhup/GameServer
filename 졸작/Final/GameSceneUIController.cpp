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
	InitPartyWindow();
}

void GameSceneUIController::InitTargetHpBar()
{
	constexpr float BARBACK_ASPECT = 39.0f / 785.0f;
	constexpr float HPBAR_WIDTH_RATIO = 692.0f / 785.0f;
	constexpr float HPBAR_HEIGHT_RATIO = 18.0f / 39.0f;
	constexpr float HPBAR_OFFSET_X = 49.0f / 785.0f;
	constexpr float HPBAR_OFFSET_Y = 11.0f / 39.0f;

	float backWidth = WinSize.x * 0.156f;
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

	const float ribbonWidth  = WinSize.x * 0.21f;
	const float ribbonHeight = WinSize.y * 0.105f;
	const float leftPageCenterX = WinSize.x * 0.30f;
	const float ribbonX = leftPageCenterX - ribbonWidth * 0.5f;
	const float ribbonY = WinSize.y * 0.78f;

	statusRibbon = make_shared<ImageUI>(L"StatusRibbon", ImageUIState::Hidden);
	statusRibbon->Init(uiManager);
	statusRibbon->SetPosition(ribbonX, ribbonY);
	statusRibbon->SetHoriLength(ribbonWidth);
	statusRibbon->SetVertLength(ribbonHeight);
	widgets.push_back(statusRibbon);

	const float arrowSizeX = WinSize.y * 0.07f;
	const float arrowSizeY = WinSize.y * 0.09f;
	const float arrowY = ribbonY + (ribbonHeight - arrowSizeY) * 0.5f;
	const float arrowGap = WinSize.x * 0.005f;

	statusArrowLeft = make_shared<ImageUI>(L"StatusArrowLeft", ImageUIState::Hidden);
	statusArrowLeft->Init(uiManager);
	statusArrowLeft->SetPosition(ribbonX - arrowSizeX - arrowGap, arrowY);
	statusArrowLeft->SetHoriLength(arrowSizeX);
	statusArrowLeft->SetVertLength(arrowSizeY);
	statusArrowLeft->SetHoverScale(1.15f);
	widgets.push_back(statusArrowLeft);

	statusArrowRight = make_shared<ImageUI>(L"StatusArrowRight", ImageUIState::Hidden);
	statusArrowRight->Init(uiManager);
	statusArrowRight->SetPosition(ribbonX + ribbonWidth + arrowGap, arrowY);
	statusArrowRight->SetHoriLength(arrowSizeX);
	statusArrowRight->SetVertLength(arrowSizeY);
	statusArrowRight->SetHoverScale(1.15f);
	widgets.push_back(statusArrowRight);

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
	default: return;
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

void GameSceneUIController::InitPartyWindow()
{
	if (sceneType != SceneType::Plaza) return;

	const float partyHeight = WinSize.y * 0.6f;
	const float partyWidth  = partyHeight * (843.0f / 720.0f);
	const float partyX = (WinSize.x - partyWidth) * 0.5f;
	const float partyY = (WinSize.y - partyHeight) * 0.5f;

	partyBook = make_shared<ImageUI>(L"PartyBook", ImageUIState::Hidden);
	partyBook->Init(uiManager);
	partyBook->SetPosition(partyX, partyY);
	partyBook->SetHoriLength(partyWidth);
	partyBook->SetVertLength(partyHeight);
	widgets.push_back(partyBook);
}

void GameSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	if (INPUT.GetKeyDown('K'))
	{
		ImageUIState next = (statusImage->GetState() == ImageUIState::Hidden)
			? ImageUIState::Visible : ImageUIState::Hidden;

		statusImage->ChangeState(next);
		statusRibbon->ChangeState(next);
		statusArrowLeft->ChangeState(next);
		statusArrowRight->ChangeState(next);
	}

	if (INPUT.GetKeyDown('P') && partyBook)
	{
		ImageUIState next = (partyBook->GetState() == ImageUIState::Hidden)
			? ImageUIState::Visible : ImageUIState::Hidden;

		partyBook->ChangeState(next);
	}

	if (statusImage->GetState() != ImageUIState::Hidden)
	{
		statusArrowLeft->SetHovered(statusArrowLeft->IsMouseInside());
		statusArrowRight->SetHovered(statusArrowRight->IsMouseInside());

		if (statusArrowLeft->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			OutputDebugStringA("[Stat] Left arrow clicked\n");
		}
		if (statusArrowRight->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			OutputDebugStringA("[Stat] Right arrow clicked\n");
		}
	}

	if (INPUT.GetKeyDown('M') && mapImage && mapBackImage)
	{
		ImageUIState next = (mapImage->GetState() == ImageUIState::Hidden)
			? ImageUIState::Visible : ImageUIState::Hidden;

		mapBackImage->ChangeState(next);
		mapImage->ChangeState(next);
	}
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
