#include "pch.h"
#include "GameSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Input.h"
#include "TextUI.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include "SoundManager.h"

GameSceneUIController::GameSceneUIController(SceneType type) : sceneType(type) {}

void GameSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	InitTargetHpBar();
	InitLocalPlayerHUD();
	InitMapNameOverlay();
	InitPartyWindow();
	InitStatWindow();
	InitMapWindow();
	InitEscWindow();
	InitKeyGuide();
	InitSettingWindow();
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

	charHPBarBack = make_shared<ImageUI>(uiManager, L"BarBack", ImageUIState::Visible);
	charHPBarBack->SetPosition(backPosX, backPosY);
	charHPBarBack->SetHoriLength(backWidth);
	charHPBarBack->SetVertLength(backHeight);
	widgets.push_back(charHPBarBack);

	float hpBarWidth = backWidth * HPBAR_WIDTH_RATIO;
	float hpBarHeight = backHeight * HPBAR_HEIGHT_RATIO;
	float hpBarPosX = backPosX + backWidth * HPBAR_OFFSET_X;
	float hpBarPosY = backPosY + backHeight * HPBAR_OFFSET_Y;

	charHPBar = make_shared<ImageUI>(uiManager, L"HpBar2", ImageUIState::Visible);
	charHPBar->SetPosition(hpBarPosX, hpBarPosY);
	charHPBar->SetHoriLength(hpBarWidth);
	charHPBar->SetVertLength(hpBarHeight);
	widgets.push_back(charHPBar);
}

void GameSceneUIController::InitLocalPlayerHUD()
{
	localCharBarsBack = make_shared<ImageUI>(uiManager, L"LocalCharBarsBack", ImageUIState::Visible);
	localCharBarsBack->SetPosition(WinSize.x * 0.02f, WinSize.y * 0.03f);
	localCharBarsBack->SetHoriLength(WinSize.y * 0.512);
	localCharBarsBack->SetVertLength(WinSize.y * 0.1f);
	widgets.push_back(localCharBarsBack);

	localCharHpBar = make_shared<ImageUI>(uiManager, L"HpBar", ImageUIState::Visible);
	localCharHpBar->SetPosition(WinSize.x * 0.0758f, WinSize.y * 0.0621f);
	localCharHpBar->SetHoriLength(WinSize.y * 0.3457);
	localCharHpBar->SetVertLength(WinSize.y * 0.0095);
	widgets.push_back(localCharHpBar);

	localCharStaminaBar = make_shared<ImageUI>(uiManager, L"StaminaBar", ImageUIState::Visible);
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
	mapNameImage = make_shared<ImageUI>(uiManager, texName, ImageUIState::Hidden);
	mapNameImage->SetPosition(WinSize.x * 0.3f, WinSize.y * 0.1f);
	mapNameImage->SetHoriLength(WinSize.x * 0.4f);
	mapNameImage->SetVertLength(WinSize.y * 0.1f);
	widgets.push_back(mapNameImage);
}

void GameSceneUIController::InitPartyWindow()
{
	if (sceneType != SceneType::Plaza) return;

	const float partyHeight = WinSize.y * 0.6f;
	const float partyWidth = partyHeight * (843.0f / 720.0f);
	const float partyX = (WinSize.x - partyWidth) * 0.5f;
	const float partyY = (WinSize.y - partyHeight) * 0.5f;

	partyBook = make_shared<ImageUI>(uiManager, L"PartyBook", ImageUIState::Hidden);
	partyBook->SetPosition(partyX, partyY);
	partyBook->SetHoriLength(partyWidth);
	partyBook->SetVertLength(partyHeight);
	widgets.push_back(partyBook);

	const float listBoxWidth = partyWidth * 0.3f;
	const float listBoxHeight = listBoxWidth * (480.0f / 1980.0f);
	const float listBoxX = partyX + partyWidth * 0.12f;
	const float listBoxY = partyY + partyHeight * 0.11f;

	partyListBox = make_shared<ImageUI>(uiManager, L"PartyList", ImageUIState::Hidden);
	partyListBox->SetPosition(listBoxX, listBoxY);
	partyListBox->SetHoriLength(listBoxWidth);
	partyListBox->SetVertLength(listBoxHeight);
	widgets.push_back(partyListBox);

	const float buttonWidth = partyWidth * 0.35f;
	const float buttonHeight = buttonWidth * (300.0f / 1860.0f);
	const float buttonY = partyY + partyHeight - buttonHeight * 2.52f;
	const float buttonGap = partyWidth * 0.08f;
	const float buttonLeftX = partyX + (partyWidth - buttonWidth * 2.0f - buttonGap) * 0.5f;
	const float buttonRightX = buttonLeftX + buttonWidth + buttonGap;

	partyCreateButton = make_shared<ImageUI>(uiManager, L"PartyCreate", ImageUIState::Hidden);
	partyCreateButton->SetPosition(buttonLeftX, buttonY);
	partyCreateButton->SetHoriLength(buttonWidth);
	partyCreateButton->SetVertLength(buttonHeight);
	partyCreateButton->SetHoverScale(1.05f);
	widgets.push_back(partyCreateButton);

	partyJoinButton = make_shared<ImageUI>(uiManager, L"PartyJoin", ImageUIState::Hidden);
	partyJoinButton->SetPosition(buttonRightX, buttonY);
	partyJoinButton->SetHoriLength(buttonWidth);
	partyJoinButton->SetVertLength(buttonHeight);
	partyJoinButton->SetHoverScale(1.05f);
	widgets.push_back(partyJoinButton);
}

void GameSceneUIController::InitStatWindow()
{
	statusBackImage = make_shared<ImageUI>(uiManager, L"StatusBack", ImageUIState::Hidden);
	statusBackImage->SetHoriLength(WinSize.x);
	statusBackImage->SetVertLength(WinSize.y);
	widgets.push_back(statusBackImage);

	statusImage = make_shared<ImageUI>(uiManager, L"Status", ImageUIState::Hidden);
	statusImage->SetHoriLength(WinSize.x);
	statusImage->SetVertLength(WinSize.y);
	widgets.push_back(statusImage);

	const float ribbonWidth  = WinSize.x * 0.21f;
	const float ribbonHeight = WinSize.y * 0.105f;
	const float leftPageCenterX = WinSize.x * 0.30f;
	const float ribbonX = leftPageCenterX - ribbonWidth * 0.5f;
	const float ribbonY = WinSize.y * 0.78f;

	statusRibbon = make_shared<ImageUI>(uiManager, L"StatusRibbon", ImageUIState::Hidden);
	statusRibbon->SetPosition(ribbonX, ribbonY);
	statusRibbon->SetHoriLength(ribbonWidth);
	statusRibbon->SetVertLength(ribbonHeight);
	widgets.push_back(statusRibbon);

	const float arrowSizeX = WinSize.y * 0.07f;
	const float arrowSizeY = WinSize.y * 0.09f;
	const float arrowY = ribbonY + (ribbonHeight - arrowSizeY) * 0.5f;
	const float arrowGap = WinSize.x * 0.005f;

	statusArrowLeft = make_shared<ImageUI>(uiManager, L"StatusArrowLeft", ImageUIState::Hidden);
	statusArrowLeft->SetPosition(ribbonX - arrowSizeX - arrowGap, arrowY);
	statusArrowLeft->SetHoriLength(arrowSizeX);
	statusArrowLeft->SetVertLength(arrowSizeY);
	statusArrowLeft->SetHoverScale(1.15f);
	widgets.push_back(statusArrowLeft);

	statusArrowRight = make_shared<ImageUI>(uiManager, L"StatusArrowRight", ImageUIState::Hidden);
	statusArrowRight->SetPosition(ribbonX + ribbonWidth + arrowGap, arrowY);
	statusArrowRight->SetHoriLength(arrowSizeX);
	statusArrowRight->SetVertLength(arrowSizeY);
	statusArrowRight->SetHoverScale(1.15f);
	widgets.push_back(statusArrowRight);

	tempStatusText = make_shared<TextUI>(uiManager, L"Texture", L"MalgunGothic");
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

	mapBackImage = make_shared<ImageUI>(uiManager, L"Black", ImageUIState::Hidden);
	mapBackImage->SetPosition(0.0f, 0.0f);
	mapBackImage->SetHoriLength(WinSize.x);
	mapBackImage->SetVertLength(WinSize.y);
	mapBackImage->SetTintAlpha(0.98f);
	widgets.push_back(mapBackImage);

	mapImage = make_shared<ImageUI>(uiManager, texName, ImageUIState::Hidden);
	mapImage->SetPosition(WinSize.x * 0.2031f, 0.0f);
	mapImage->SetHoriLength(WinSize.y * (1140.0f/1080.0f));
	mapImage->SetVertLength(WinSize.y);
	widgets.push_back(mapImage);
}

void GameSceneUIController::InitEscWindow()
{
	const float escSize = WinSize.y * 0.5f;
	const float escX      = (WinSize.x - escSize) * 0.5f;
	const float escY      = (WinSize.y - escSize) * 0.5f;

	escWindow = make_shared<ImageUI>(uiManager, L"EscWindow", ImageUIState::Hidden);
	escWindow->SetPosition(escX, escY);
	escWindow->SetHoriLength(escSize);
	escWindow->SetVertLength(escSize);
	widgets.push_back(escWindow);

	const float btnWidth  = escSize * 0.5f;
	const float btnHeight = btnWidth * (480.0f / 1980.0f);
	const float btnGap    = escSize * 0.03f;
	const float btnTotalH = btnHeight * 3.0f + btnGap * 2.0f;
	const float btnStartY = escY + (escSize - btnTotalH) * 0.55f;
	const float btnX      = escX + (escSize - btnWidth) * 0.5f;

	auto makeEscButton = [&](shared_ptr<ImageUI>& target, const wstring& tex, int slot) {
		target = make_shared<ImageUI>(uiManager, tex, ImageUIState::Hidden);
		target->SetPosition(btnX, btnStartY + (btnHeight + btnGap) * slot);
		target->SetHoriLength(btnWidth);
		target->SetVertLength(btnHeight);
		target->SetHoverScale(1.05f);
		widgets.push_back(target);
	};

	makeEscButton(escContinueButton, L"ESCContinue", 0);
	makeEscButton(escOptionsButton,  L"ESCSetting",  1);
	makeEscButton(escExitButton,     L"ESCQuit",     2);
}

void GameSceneUIController::InitKeyGuide()
{
	const float guideHeight = WinSize.y * 0.6f;
	const float guideWidth = guideHeight * (843.0f / 720.0f);
	const float guideX = (WinSize.x - guideWidth) * 0.5f;
	const float guideY = (WinSize.y - guideHeight) * 0.5f;

	keyGuide = make_shared<ImageUI>(uiManager, L"KeyGuide", ImageUIState::Hidden);
	keyGuide->SetPosition(guideX, guideY);
	keyGuide->SetHoriLength(guideWidth);
	keyGuide->SetVertLength(guideHeight);
	widgets.push_back(keyGuide);
}

void GameSceneUIController::InitSettingWindow()
{
	const float backSize = WinSize.y * 0.8f;
	const float backX = (WinSize.x - backSize) * 0.5f;
	const float backY = (WinSize.y - backSize) * 0.5f;

	settingWindow = make_shared<ImageUI>(uiManager, L"SettingWindow", ImageUIState::Hidden);
	settingWindow->SetPosition(backX, backY);
	settingWindow->SetHoriLength(backSize);
	settingWindow->SetVertLength(backSize);
	widgets.push_back(settingWindow);

	const float btnW   = backSize * 0.22f;
	const float btnH   = btnW * (1056.0f / 4096.0f);
	const float margin = backSize * 0.045f;
	const float btnX   = backX + backSize - btnW - margin;
	const float btnY   = backY + margin;

	settingBackButton = make_shared<ImageUI>(uiManager, L"PartyBack", ImageUIState::Hidden);
	settingBackButton->SetPosition(btnX, btnY);
	settingBackButton->SetHoriLength(btnW);
	settingBackButton->SetVertLength(btnH);
	settingBackButton->SetHoverScale(1.05f);
	widgets.push_back(settingBackButton);
}

void GameSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	auto opened = [](const shared_ptr<ImageUI>& p) {
		return p && p->GetState() != ImageUIState::Hidden;
	};

	if (INPUT.GetKeyDown('K'))
	{
		bool selfOpen = opened(statusImage);
		bool othersOpen = opened(escWindow) || opened(partyBook) || opened(mapImage) || opened(keyGuide) || opened(settingWindow);
		if (selfOpen || !othersOpen)
		{
			ImageUIState next = selfOpen ? ImageUIState::Hidden : ImageUIState::Visible;

			statusBackImage->ChangeState(next);
			statusImage->ChangeState(next);
			statusRibbon->ChangeState(next);
			statusArrowLeft->ChangeState(next);
			statusArrowRight->ChangeState(next);

			SCENE_MANAGER->GetCurrentScene()->GetCamera()->SetCursor(next == ImageUIState::Visible);
		}
	}

	if (INPUT.GetKeyDown(VK_ESCAPE) && escWindow)
	{
		bool selfOpen = opened(escWindow);
		bool othersOpen = opened(statusImage) || opened(partyBook) || opened(mapImage) || opened(keyGuide) || opened(settingWindow);
		if (selfOpen || !othersOpen)
		{
			ImageUIState next = selfOpen ? ImageUIState::Hidden : ImageUIState::Visible;

			escWindow->ChangeState(next);
			escContinueButton->ChangeState(next);
			escOptionsButton->ChangeState(next);
			escExitButton->ChangeState(next);

			SCENE_MANAGER->GetCurrentScene()->GetCamera()->SetCursor(next == ImageUIState::Visible);
		}
	}

	if (escWindow && escWindow->GetState() != ImageUIState::Hidden)
	{
		escContinueButton->SetHovered(escContinueButton->IsMouseInside());
		escOptionsButton->SetHovered(escOptionsButton->IsMouseInside());
		escExitButton->SetHovered(escExitButton->IsMouseInside());

		if (escContinueButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			escWindow->ChangeState(ImageUIState::Hidden);
			escContinueButton->ChangeState(ImageUIState::Hidden);
			escOptionsButton->ChangeState(ImageUIState::Hidden);
			escExitButton->ChangeState(ImageUIState::Hidden);

			SCENE_MANAGER->GetCurrentScene()->GetCamera()->SetCursor(false);
		}
		if (escOptionsButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			escWindow->ChangeState(ImageUIState::Hidden);
			escContinueButton->ChangeState(ImageUIState::Hidden);
			escOptionsButton->ChangeState(ImageUIState::Hidden);
			escExitButton->ChangeState(ImageUIState::Hidden);

			if (settingWindow)     settingWindow->ChangeState(ImageUIState::Visible);
			if (settingBackButton) settingBackButton->ChangeState(ImageUIState::Visible);
		}
		if (escExitButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			PostQuitMessage(0);
		}
	}

	if (settingWindow && settingWindow->GetState() != ImageUIState::Hidden)
	{
		settingBackButton->SetHovered(settingBackButton->IsMouseInside());

		if (settingBackButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			settingWindow->ChangeState(ImageUIState::Hidden);
			settingBackButton->ChangeState(ImageUIState::Hidden);

			escWindow->ChangeState(ImageUIState::Visible);
			escContinueButton->ChangeState(ImageUIState::Visible);
			escOptionsButton->ChangeState(ImageUIState::Visible);
			escExitButton->ChangeState(ImageUIState::Visible);
		}
	}

	if (INPUT.GetKeyDown('P') && partyBook)
	{
		bool selfOpen = opened(partyBook);
		bool othersOpen = opened(escWindow) || opened(statusImage) || opened(mapImage) || opened(keyGuide) || opened(settingWindow);
		if (selfOpen || !othersOpen)
		{
			ImageUIState next = selfOpen ? ImageUIState::Hidden : ImageUIState::Visible;

			partyBook->ChangeState(next);
			if (partyListBox)      partyListBox->ChangeState(next);
			if (partyCreateButton) partyCreateButton->ChangeState(next);
			if (partyJoinButton)   partyJoinButton->ChangeState(next);

			SCENE_MANAGER->GetCurrentScene()->GetCamera()->SetCursor(next == ImageUIState::Visible);
		}
	}

	if (partyBook && partyBook->GetState() != ImageUIState::Hidden)
	{
		partyCreateButton->SetHovered(partyCreateButton->IsMouseInside());
		partyJoinButton->SetHovered(partyJoinButton->IsMouseInside());

		if (partyCreateButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			OutputDebugStringA("[Party] Create button clicked\n");
		}
		if (partyJoinButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			OutputDebugStringA("[Party] Join button clicked\n");
		}
	}

	if (statusImage->GetState() != ImageUIState::Hidden)
	{
		statusArrowLeft->SetHovered(statusArrowLeft->IsMouseInside());
		statusArrowRight->SetHovered(statusArrowRight->IsMouseInside());

		if (statusArrowLeft->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			OutputDebugStringA("[Stat] Left arrow clicked\n");
		}
		if (statusArrowRight->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			OutputDebugStringA("[Stat] Right arrow clicked\n");
		}
	}

	if (INPUT.GetKeyDown('M') && mapImage && mapBackImage)
	{
		bool selfOpen = opened(mapImage);
		bool othersOpen = opened(escWindow) || opened(statusImage) || opened(partyBook) || opened(keyGuide) || opened(settingWindow);
		if (selfOpen || !othersOpen)
		{
			ImageUIState next = selfOpen ? ImageUIState::Hidden : ImageUIState::Visible;

			mapBackImage->ChangeState(next);
			mapImage->ChangeState(next);

			SCENE_MANAGER->GetCurrentScene()->GetCamera()->SetCursor(next == ImageUIState::Visible);
		}
	}

	if (INPUT.GetKeyDown(VK_F2) && keyGuide)
	{
		bool selfOpen = opened(keyGuide);
		bool othersOpen = opened(escWindow) || opened(statusImage) || opened(partyBook) || opened(mapImage) || opened(settingWindow);
		if (selfOpen || !othersOpen)
		{
			ImageUIState next = selfOpen ? ImageUIState::Hidden : ImageUIState::Visible;

			keyGuide->ChangeState(next);
			SCENE_MANAGER->GetCurrentScene()->GetCamera()->SetCursor(next == ImageUIState::Visible);
		}
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
	wstring text =
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
