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
#include "ClientPartyState.h"

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
	InitJoinRequestPopup();
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

	localCharPotion = make_shared<ImageUI>(uiManager, L"Potion", ImageUIState::Visible);
	localCharPotion->SetPosition(WinSize.x * 0.023f, WinSize.y * 0.75f);
	localCharPotion->SetHoriLength(WinSize.y * 0.2176f);
	localCharPotion->SetVertLength(WinSize.y * 0.1952f);
	widgets.push_back(localCharPotion);
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
	const float textScale = WinSize.y / 1080.0f;

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

	partyMyPartyBox = make_shared<ImageUI>(uiManager, L"PartyMyParty", ImageUIState::Hidden);
	partyMyPartyBox->SetPosition(listBoxX, listBoxY);
	partyMyPartyBox->SetHoriLength(listBoxWidth);
	partyMyPartyBox->SetVertLength(listBoxHeight);
	widgets.push_back(partyMyPartyBox);

	const float myCardWidth  = partyWidth * 0.35f;
	const float myCardHeight = partyHeight * 0.11f;
	const float myCardGap    = partyHeight * 0.025f;
	const float myCardStartY = listBoxY + listBoxHeight + partyHeight * 0.04f;

	partyMyCards.reserve(MAX_PARTY_MEMBERS);
	partyMyLabels.reserve(MAX_PARTY_MEMBERS);
	for (int i = 0; i < MAX_PARTY_MEMBERS; ++i)
	{
		const float cardY = myCardStartY + i * (myCardHeight + myCardGap);

		auto card = make_shared<ImageUI>(uiManager, L"PartyKnight", ImageUIState::Hidden);
		card->SetPosition(listBoxX, cardY);
		card->SetHoriLength(myCardWidth);
		card->SetVertLength(myCardHeight);
		widgets.push_back(card);
		partyMyCards.push_back(card);

		auto label = make_shared<TextUI>(uiManager, L"PartyMyCardLabel", L"VerdanaBold");
		label->SetPosition(listBoxX + myCardWidth * 0.25f, cardY + myCardHeight * 0.2f);
		label->SetScale(0.4f * textScale);
		widgets.push_back(label);
		partyMyLabels.push_back(label);
	}

	const float cardWidth  = partyWidth * 0.35f;
	const float cardHeight = partyHeight * 0.11f;
	const float cardGap    = partyHeight * 0.025f;
	const float cardStartY = listBoxY + listBoxHeight + partyHeight * 0.04f;

	partyListCards.reserve(MAX_PARTY_CARDS);
	partyListLabels.reserve(MAX_PARTY_CARDS);
	partyListCountLabels.reserve(MAX_PARTY_CARDS);
	partyListCardIds.assign(MAX_PARTY_CARDS, 0);
	for (int i = 0; i < MAX_PARTY_CARDS; ++i)
	{
		const float cardY = cardStartY + i * (cardHeight + cardGap);

		auto card = make_shared<ImageUI>(uiManager, L"PartyKnight", ImageUIState::Hidden);
		card->SetPosition(listBoxX, cardY);
		card->SetHoriLength(cardWidth);
		card->SetVertLength(cardHeight);
		card->SetHoverScale(1.05f);
		widgets.push_back(card);
		partyListCards.push_back(card);

		auto label = make_shared<TextUI>(uiManager, L"PartyCardLabel", L"VerdanaBold");
		label->SetPosition(listBoxX + cardWidth * 0.25f, cardY + cardHeight * 0.2f);
		label->SetScale(0.4f * textScale);
		widgets.push_back(label);
		partyListLabels.push_back(label);

		auto countLabel = make_shared<TextUI>(uiManager, L"PartyCardCount", L"VerdanaBold");
		countLabel->SetPosition(listBoxX + cardWidth * 0.55f, cardY + cardHeight * 0.5f);
		countLabel->SetScale(0.4f * textScale);
		widgets.push_back(countLabel);
		partyListCountLabels.push_back(countLabel);
	}
}

void GameSceneUIController::ShowPartyView(PartyView view)
{
	partyView = view;
	const ImageUIState lobby   = (view == PartyView::Lobby)   ? ImageUIState::Visible : ImageUIState::Hidden;
	const ImageUIState created = (view == PartyView::Created) ? ImageUIState::Visible : ImageUIState::Hidden;

	if (partyListBox)      partyListBox->ChangeState(lobby);
	if (partyCreateButton) partyCreateButton->ChangeState(lobby);
	if (partyJoinButton)   partyJoinButton->ChangeState(lobby);
	if (partyMyPartyBox)   partyMyPartyBox->ChangeState(created);

	if (view == PartyView::Created)
	{
		RefreshMyPartyText();

		for (auto& card : partyListCards)
			if (card) card->ChangeState(ImageUIState::Hidden);
		for (auto& label : partyListLabels)
			if (label) label->SetText(L"");
		for (auto& count : partyListCountLabels)
			if (count) count->SetText(L"");
	}
	else
	{
		for (auto& card : partyMyCards)
			if (card) card->ChangeState(ImageUIState::Hidden);
		for (auto& label : partyMyLabels)
			if (label) label->SetText(L"");
		selectedPartyId = 0;
		RefreshPartyList();
	}
}

void GameSceneUIController::RefreshPartyList()
{
	if (partyListCards.empty()) return;

	ClientPartyState* party = ENGINE.GetPartyState();

	vector<const Protocol::PartyListEntry*> sorted;
	if (party)
	{
		for (const Protocol::PartyListEntry& e : party->GetPartyList())
		{
			sorted.push_back(&e);
		}
		std::sort(sorted.begin(), sorted.end(),
			[](const Protocol::PartyListEntry* a, const Protocol::PartyListEntry* b)
			{
				return a->createdatsec() < b->createdatsec();
			});
	}

	for (int i = 0; i < MAX_PARTY_CARDS; ++i)
	{
		const bool hasEntry = i < static_cast<int>(sorted.size());

		if (!hasEntry)
		{
			partyListCards[i]->ChangeState(ImageUIState::Hidden);
			partyListLabels[i]->SetText(L"");
			partyListCountLabels[i]->SetText(L"");
			partyListCardIds[i] = 0;
			continue;
		}

		const Protocol::PartyListEntry& entry = *sorted[i];
		partyListCardIds[i] = entry.partyid();

		const CharacterType leaderClass = CharacterType::Knight;
		const wchar_t* texture =
			(leaderClass == CharacterType::Lancer)  ? L"PartyLancer"  :
			(leaderClass == CharacterType::Paladin) ? L"PartyPaladin" :
			                                          L"PartyKnight";

		partyListCards[i]->SetTexture(texture);
		partyListCards[i]->ChangeState(ImageUIState::Visible);

		partyListLabels[i]->SetText(L"Party #" + std::to_wstring(entry.partyid()));
		partyListCountLabels[i]->SetText(
			std::to_wstring(entry.membercount()) + L"/" + std::to_wstring(entry.capacity()));
	}

	if (selectedPartyId != 0 &&
		std::find(partyListCardIds.begin(), partyListCardIds.end(), selectedPartyId) == partyListCardIds.end())
	{
		selectedPartyId = 0;
	}
}

void GameSceneUIController::RefreshMyPartyText()
{
	if (partyMyCards.empty()) return;

	ClientPartyState* party = ENGINE.GetPartyState();

	vector<const Protocol::PartyMember*> ordered;
	if (party && party->HasMyParty())
	{
		const Protocol::PartySnapshot& snapshot = party->GetMyParty();
		ordered.reserve(snapshot.members_size());
		for (int i = 0; i < snapshot.members_size(); ++i)
			if (snapshot.members(i).role() == Protocol::PARTY_MEMBER_ROLE_LEADER)
				ordered.push_back(&snapshot.members(i));
		for (int i = 0; i < snapshot.members_size(); ++i)
			if (snapshot.members(i).role() != Protocol::PARTY_MEMBER_ROLE_LEADER)
				ordered.push_back(&snapshot.members(i));
	}

	for (int i = 0; i < MAX_PARTY_MEMBERS; ++i)
	{
		if (i >= static_cast<int>(ordered.size()))
		{
			partyMyCards[i]->ChangeState(ImageUIState::Hidden);
			partyMyLabels[i]->SetText((i == 0 && ordered.empty()) ? L"Creating party..." : L"");
			continue;
		}

		const Protocol::PartyMember* member = ordered[i];
		const CharacterType memberClass = static_cast<CharacterType>(member->charactertype());
		const wchar_t* texture =
			(memberClass == CharacterType::Lancer)  ? L"PartyLancer"  :
			(memberClass == CharacterType::Paladin) ? L"PartyPaladin" :
			                                          L"PartyKnight";
		const wchar_t* className =
			(memberClass == CharacterType::Lancer)  ? L"Lancer"  :
			(memberClass == CharacterType::Paladin) ? L"Paladin" :
			                                          L"Knight";

		partyMyCards[i]->SetTexture(texture);
		partyMyCards[i]->ChangeState(ImageUIState::Visible);
		partyMyLabels[i]->SetText(
			L"ID: " + std::to_wstring(member->sessionid()) + L"\nClass: " + className);
	}
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

void GameSceneUIController::InitJoinRequestPopup()
{
	const float popupW = WinSize.y * 0.36f;
	const float popupH = popupW;
	const float margin = WinSize.x * 0.02f;
	const float popupX = WinSize.x - popupW - margin;
	const float popupY = (WinSize.y - popupH) * 0.5f + WinSize.y * 0.06f;
	const float textScale = WinSize.y / 1080.0f;

	joinRequestWindow = make_shared<ImageUI>(uiManager, L"EscWindow", ImageUIState::Hidden);
	joinRequestWindow->SetPosition(popupX, popupY);
	joinRequestWindow->SetHoriLength(popupW);
	joinRequestWindow->SetVertLength(popupH);
	widgets.push_back(joinRequestWindow);

	joinRequestText = make_shared<TextUI>(uiManager, L"PartyJoinRequestText", L"VerdanaBold");
	joinRequestText->SetPosition(popupX + popupW * 0.14f, popupY + popupH * 0.32f);
	joinRequestText->SetScale(0.4f * textScale);
	widgets.push_back(joinRequestText);

	const float btnW = popupW * 0.34f;
	const float btnH = btnW * (480.0f / 1980.0f);
	const float btnY = popupY + popupH - btnH - popupH * 0.26f;

	joinRequestOkButton = make_shared<ImageUI>(uiManager, L"OK", ImageUIState::Hidden);
	joinRequestOkButton->SetPosition(popupX + popupW * 0.1f, btnY);
	joinRequestOkButton->SetHoriLength(btnW);
	joinRequestOkButton->SetVertLength(btnH);
	joinRequestOkButton->SetHoverScale(1.05f);
	widgets.push_back(joinRequestOkButton);

	joinRequestCancelButton = make_shared<ImageUI>(uiManager, L"CANCEL", ImageUIState::Hidden);
	joinRequestCancelButton->SetPosition(popupX + popupW - btnW - popupW * 0.1f, btnY);
	joinRequestCancelButton->SetHoriLength(btnW);
	joinRequestCancelButton->SetVertLength(btnH);
	joinRequestCancelButton->SetHoverScale(1.05f);
	widgets.push_back(joinRequestCancelButton);

	joinSlideWidgets = { joinRequestWindow, joinRequestText, joinRequestOkButton, joinRequestCancelButton };
	joinSlideBaseX.clear();
	for (const auto& w : joinSlideWidgets)
		joinSlideBaseX.push_back(w->GetPosX());
	joinSlideDist = WinSize.x - popupX;   
	joinSlideElapsed = JOIN_SLIDE_DURATION;
}

bool GameSceneUIController::IsMyPartyLeader() const
{
	ClientPartyState* party = ENGINE.GetPartyState();
	if (!party || !party->HasMyParty()) return false;

	const Protocol::PartySnapshot& snapshot = party->GetMyParty();
	for (int i = 0; i < snapshot.members_size(); ++i)
		if (snapshot.members(i).role() == Protocol::PARTY_MEMBER_ROLE_LEADER)
			return snapshot.members(i).netid() == static_cast<uint64_t>(INPUT.GetClientID());
	return false;
}

void GameSceneUIController::UpdateJoinRequestPopup(float deltaTime)
{
	if (!joinRequestWindow) return;

	ClientPartyState* party = ENGINE.GetPartyState();
	const bool popupOpen = joinRequestWindow->GetState() != ImageUIState::Hidden;

	const Protocol::PartyJoinRequest* active = nullptr;
	if (party && IsMyPartyLeader() && !party->GetPendingJoinRequests().empty())
	{
		for (const Protocol::PartyJoinRequest& req : party->GetPendingJoinRequests())
			if (req.joinrequestid() == activeJoinRequestId) { active = &req; break; }
		if (!active)
			active = &party->GetPendingJoinRequests().front();
	}

	if (!active)
	{
		if (popupOpen)
		{
			joinRequestWindow->ChangeState(ImageUIState::Hidden);
			joinRequestOkButton->ChangeState(ImageUIState::Hidden);
			joinRequestCancelButton->ChangeState(ImageUIState::Hidden);
			joinRequestText->SetText(L"");
			activeJoinRequestId = 0;
		}
		return;
	}

	if (!popupOpen || active->joinrequestid() != activeJoinRequestId)
	{
		joinSlideElapsed = 0.0f;   
		activeJoinRequestId = active->joinrequestid();

		wstring text = L"Party Join Request\n";
		text += L"ID: " + std::to_wstring(active->requestersessionid()) + L"\n";
		text += L"Class: -";
		joinRequestText->SetText(text);

		joinRequestWindow->ChangeState(ImageUIState::Visible);
		joinRequestOkButton->ChangeState(ImageUIState::Visible);
		joinRequestCancelButton->ChangeState(ImageUIState::Visible);
	}

	if (joinSlideElapsed < JOIN_SLIDE_DURATION)
	{
		joinSlideElapsed = min(joinSlideElapsed + deltaTime, JOIN_SLIDE_DURATION);
		const float t = joinSlideElapsed / JOIN_SLIDE_DURATION;
		const float eased = 1.0f - (1.0f - t) * (1.0f - t);  
		const float offset = (1.0f - eased) * joinSlideDist;
		for (size_t i = 0; i < joinSlideWidgets.size(); ++i)
			joinSlideWidgets[i]->SetPosition(joinSlideBaseX[i] + offset, joinSlideWidgets[i]->GetPosY());
	}

	joinRequestOkButton->SetHovered(joinRequestOkButton->IsMouseInside());
	joinRequestCancelButton->SetHovered(joinRequestCancelButton->IsMouseInside());

	if (joinRequestOkButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		if (auto* network = NETWORK_MANAGER)
			network->SendPartyJoinAcceptPacket(activeJoinRequestId);
	}
	else if (joinRequestCancelButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		if (auto* network = NETWORK_MANAGER)
			network->SendPartyJoinRejectPacket(activeJoinRequestId);
	}
}

void GameSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	UpdateJoinRequestPopup(deltaTime);

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
			if (selfOpen)
			{
				partyBook->ChangeState(ImageUIState::Hidden);
				if (partyListBox)      partyListBox->ChangeState(ImageUIState::Hidden);
				if (partyCreateButton) partyCreateButton->ChangeState(ImageUIState::Hidden);
				if (partyJoinButton)   partyJoinButton->ChangeState(ImageUIState::Hidden);
				if (partyMyPartyBox)   partyMyPartyBox->ChangeState(ImageUIState::Hidden);
				for (auto& card : partyMyCards)
					if (card) card->ChangeState(ImageUIState::Hidden);
				for (auto& label : partyMyLabels)
					if (label) label->SetText(L"");
				for (auto& card : partyListCards)
					if (card) card->ChangeState(ImageUIState::Hidden);
				for (auto& label : partyListLabels)
					if (label) label->SetText(L"");
				for (auto& count : partyListCountLabels)
					if (count) count->SetText(L"");
			}
			else
			{
				partyBook->ChangeState(ImageUIState::Visible);
				if (auto* network = NETWORK_MANAGER)
				{
					network->SendPartyUiOpenedPacket();
				}
				ClientPartyState* party = ENGINE.GetPartyState();
				ShowPartyView(party && party->HasMyParty() ? PartyView::Created : PartyView::Lobby);
			}
		}
	}

	if (partyBook && partyBook->GetState() != ImageUIState::Hidden)
	{
		if (partyView == PartyView::Lobby)
		{
			if (ClientPartyState* party = ENGINE.GetPartyState())
			{
				if (party->HasMyParty())
				{
					ShowPartyView(PartyView::Created);
					return;
				}

				if (party->GetRevision() != lastPartyRevision)
				{
					lastPartyRevision = party->GetRevision();
					RefreshPartyList();
				}
			}

			for (int i = 0; i < MAX_PARTY_CARDS; ++i)
			{
				shared_ptr<ImageUI>& card = partyListCards[i];
				if (!card || card->GetState() == ImageUIState::Hidden) continue;

				const bool selected = (partyListCardIds[i] != 0 && partyListCardIds[i] == selectedPartyId);
				card->SetHovered(card->IsMouseInside() || selected);

				if (card->IsMouseInside() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
				{
					SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
					selectedPartyId = partyListCardIds[i];
				}
			}

			partyCreateButton->SetHovered(partyCreateButton->IsMouseInside());
			partyJoinButton->SetHovered(partyJoinButton->IsMouseInside());

			if (partyCreateButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
			{
				SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
				if (auto* network = NETWORK_MANAGER)
				{
					network->SendPartyCreatePacket();
				}
				ShowPartyView(PartyView::Created);
			}
			if (partyJoinButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
			{
				SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
				if (selectedPartyId != 0)
				{
					if (auto* network = NETWORK_MANAGER)
					{
						network->SendPartyJoinRequestPacket(selectedPartyId);
					}
				}
				else
				{
					OutputDebugStringA("[Party] Join clicked but no party selected\n");
				}
			}
		}
		else
		{
			if (ClientPartyState* party = ENGINE.GetPartyState())
			{
				if (party->GetRevision() != lastPartyRevision)
				{
					lastPartyRevision = party->GetRevision();
					RefreshMyPartyText();
				}
			}
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
		}
	}

	const bool wantCursor =
		opened(statusImage) || opened(escWindow)   || opened(partyBook) ||
		opened(mapImage)    || opened(keyGuide)    || opened(settingWindow) ||
		opened(joinRequestWindow);
	if (Camera* camera = SCENE_MANAGER->GetCurrentScene()->GetCamera())
		if (camera->IsCursorActive() != wantCursor)
			camera->SetCursor(wantCursor);
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
		L"CurHp: " + to_wstring(curHp) + L"\n" +
		L"MaxHp: " + to_wstring(maxHp) + L"\n" +
		L"CurStamina: " + to_wstring(curStamina) + L"\n" +
		L"MaxStamina: " + to_wstring(maxStamina) + L"\n" +
		L"Power: " + to_wstring(power) + L"\n" +
		L"aSpeed: " + to_wstring(aSpeed) + L"\n" +
		L"Defense: " + to_wstring(defense) + L"\n" +
		L"mSpeed: " + to_wstring(mSpeed) + L"\n";
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
