#include "pch.h"
#include "GameSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Input.h"
#include "TextUI.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "MainCharacter.h"
#include "AnimationMachine.h"
#include "Animator.h"
#include "Camera.h"
#include "SoundManager.h"
#include "ImGuiManager.h"
#include "ClientPartyState.h"
#include "ClientTitleState.h"
#include "ClientWorldTransitionController.h"
#include "NetworkManager.h"
#include "NetId.h"

GameSceneUIController::GameSceneUIController(SceneType type) : sceneType(type) {}

void GameSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	InitInteractPrompt();
	InitStatueWindow();
	InitMonsterHpBars();
	InitBossHpBar();
	InitBeaconWindow();
	InitHeroChoiceWindow();
	InitRespawnWindow();
	InitLocalPlayerHUD();
	InitMapNameOverlay();
	InitPartyMemberHud();   
	InitPartyWindow();
	InitStatWindow();
	InitMapWindow();
	InitEscWindow();
	InitKeyGuide();
	InitSettingWindow();
	InitJoinRequestPopup();
}

void GameSceneUIController::InitMonsterHpBars()
{
	monsterBarBacks.reserve(MAX_MONSTER_HP_BARS);
	monsterBars.reserve(MAX_MONSTER_HP_BARS);
	for (int i = 0; i < MAX_MONSTER_HP_BARS; ++i)
	{
		auto back = make_shared<ImageUI>(uiManager, L"BarBack", ImageUIState::Hidden);
		widgets.push_back(back);
		monsterBarBacks.push_back(back);

		auto bar = make_shared<ImageUI>(uiManager, L"HpBar2", ImageUIState::Hidden);
		widgets.push_back(bar);
		monsterBars.push_back(bar);
	}
}

void GameSceneUIController::InitLocalPlayerHUD()
{
	localCharBarsBack = make_shared<ImageUI>(uiManager, L"LocalCharBarsBack", ImageUIState::Visible);
	localCharBarsBack->SetPosition(WinSize.x * 0.02f, WinSize.y * 0.03f);
	localCharBarsBack->SetHoriLength(WinSize.y * 0.512f);
	localCharBarsBack->SetVertLength(WinSize.y * 0.1f);
	widgets.push_back(localCharBarsBack);

	localCharHpBar = make_shared<ImageUI>(uiManager, L"HpBar", ImageUIState::Visible);
	localCharHpBar->SetPosition(WinSize.x * 0.0758f, WinSize.y * 0.0621f);
	localCharHpBar->SetHoriLength(WinSize.y * 0.3457f);
	localCharHpBar->SetVertLength(WinSize.y * 0.0095f);
	widgets.push_back(localCharHpBar);

	localCharStaminaBar = make_shared<ImageUI>(uiManager, L"StaminaBar", ImageUIState::Visible);
	localCharStaminaBar->SetPosition(WinSize.x * 0.0767f, WinSize.y * 0.087499f);
	localCharStaminaBar->SetHoriLength(WinSize.y * 0.2566f);
	localCharStaminaBar->SetVertLength(WinSize.y * 0.00626f);
	widgets.push_back(localCharStaminaBar);

	const bool showPotion = (sceneType != SceneType::Plaza);

	localCharPotion = make_shared<ImageUI>(uiManager, L"Potion",
		showPotion ? ImageUIState::Visible : ImageUIState::Hidden);
	localCharPotion->SetPosition(WinSize.x * 0.023f, WinSize.y * 0.75f);
	localCharPotion->SetHoriLength(WinSize.y * 0.2176f);
	localCharPotion->SetVertLength(WinSize.y * 0.1952f);
	widgets.push_back(localCharPotion);

	localCharPotionCount = make_shared<TextUI>(uiManager, L"PotionCount", L"VerdanaBold");
	const float potionLeft = WinSize.x * 0.023f;
	const float potionTop = WinSize.y * 0.75f;
	const float potionW = WinSize.y * 0.2176f;
	const float potionH = WinSize.y * 0.1952f;
	const float potionScale = WinSize.y / 1080.0f * 0.5f;
	localCharPotionCount->SetScale(potionScale);
	localCharPotionCount->SetTextColor(Colors::Orange);
	localCharPotionCount->SetPosition(
		potionLeft + potionW * 0.75f - 22.5f * potionScale,
		potionTop + potionH * 0.801f - 27.0f * potionScale);
	localCharPotionCount->SetText(showPotion ? L"0" : L"");
	widgets.push_back(localCharPotionCount);

	const float deathW = WinSize.y * 0.034f;   
	const float deathH = WinSize.y * 0.038f;
	const float deathLeft = WinSize.x * 0.08f;
	const float deathTop = WinSize.y * 0.11f;

	localCharDeathCount = make_shared<ImageUI>(uiManager, L"DeathCount",
		showPotion ? ImageUIState::Visible : ImageUIState::Hidden);
	localCharDeathCount->SetPosition(deathLeft, deathTop);
	localCharDeathCount->SetHoriLength(deathW);
	localCharDeathCount->SetVertLength(deathH);
	widgets.push_back(localCharDeathCount);

	localCharDeathCountText = make_shared<TextUI>(uiManager, L"DeathCountText", L"VerdanaBold");
	const float deathScale = WinSize.y / 1080.0f * 0.6f;
	localCharDeathCountText->SetScale(deathScale);
	localCharDeathCountText->SetTextColor(Colors::Red);
	localCharDeathCountText->SetPosition(
		deathLeft + deathW * 0.85f,
		deathTop + deathH * 0.5f - 27.0f * deathScale);
	localCharDeathCountText->SetText(L"");
	widgets.push_back(localCharDeathCountText);
}

void GameSceneUIController::SetPotionCount(uint32_t count)
{
	if (!localCharPotionCount) return;
	if (sceneType == SceneType::Plaza) return;
	localCharPotionCount->SetText(to_wstring(count));
}

void GameSceneUIController::SetDeathCount(uint32_t death, uint32_t max)
{
	if (!localCharDeathCountText) return;
	if (sceneType == SceneType::Plaza) return;
	localCharDeathCountText->SetText(L" x " + to_wstring(death));
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

		const CharacterType leaderClass = static_cast<CharacterType>(entry.leadercharactertype());
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

void GameSceneUIController::InitPartyMemberHud()
{
	constexpr float BACK_ASPECT        = 1562.0f / 2737.0f; 
	constexpr float BARBACK_ASPECT     = 39.0f / 785.0f;    
	constexpr float HPBAR_WIDTH_RATIO  = 692.0f / 785.0f;
	constexpr float HPBAR_HEIGHT_RATIO = 18.0f / 39.0f;
	constexpr float HPBAR_OFFSET_X     = 49.0f / 785.0f;
	constexpr float HPBAR_OFFSET_Y     = 11.0f / 39.0f;

	const float textScale = WinSize.y / 1080.0f;

	constexpr float BACK_ALPHA = 0.7f;   

	const float panelW = WinSize.x * 0.20f;
	const float panelH = panelW * BACK_ASPECT;
	const float panelX = WinSize.x - panelW - WinSize.x * 0.012f;
	const float panelY = WinSize.y * 0.05f;

	partyHudBack = make_shared<ImageUI>(uiManager, L"PartyMemBack", ImageUIState::Hidden);
	partyHudBack->SetPosition(panelX, panelY);
	partyHudBack->SetHoriLength(panelW);
	partyHudBack->SetVertLength(panelH);
	partyHudBack->SetTintAlpha(BACK_ALPHA);
	widgets.push_back(partyHudBack);

	partyHudIcons.reserve(PARTY_HUD_SLOTS);
	partyHudNames.reserve(PARTY_HUD_SLOTS);
	partyHudBarBacks.reserve(PARTY_HUD_SLOTS);
	partyHudBars.reserve(PARTY_HUD_SLOTS);
	partyHudBarFullW.assign(PARTY_HUD_SLOTS, 0.0f);
	partyHudSlotIds.assign(PARTY_HUD_SLOTS, -1);

	const float otherTop  = panelY + panelH * 0.48f;  
	const float otherRowH = panelH * 0.23f;           

	for (int i = 0; i < PARTY_HUD_SLOTS; ++i)
	{
		const bool isSelf = (i == 0);

		float iconSize, iconX, iconY, nameX, nameY, barX, barY, barW;
		if (isSelf)
		{
			iconSize = panelH * 0.27f;
			iconX = panelX + panelW * 0.18f;
			iconY = panelY + panelH * 0.11f;
			nameX = panelX + panelW * 0.35f;
			nameY = panelY + panelH * 0.13f;
			barW  = panelW * 0.53f;
			barX  = panelX + panelW * 0.35f;
			barY  = panelY + panelH * 0.30f;
		}
		else
		{
			const float rowTop = otherTop + (i - 1) * otherRowH;
			iconSize = panelH * 0.20f;
			iconX = panelX + panelW * 0.185f;
			iconY = rowTop + otherRowH * 0.03f;
			nameX = panelX + panelW * 0.31f;
			nameY = rowTop + otherRowH * 0.02f;
			barW  = panelW * 0.51f;
			barX  = panelX + panelW * 0.31f;
			barY  = rowTop + otherRowH * 0.55f;
		}

		auto icon = make_shared<ImageUI>(uiManager, L"PartyMemKnight", ImageUIState::Hidden);
		icon->SetPosition(iconX, iconY);
		icon->SetHoriLength(iconSize);
		icon->SetVertLength(iconSize);
		widgets.push_back(icon);
		partyHudIcons.push_back(icon);

		auto name = make_shared<TextUI>(uiManager, L"PartyHudName", L"VerdanaBold");
		name->SetPosition(nameX, nameY);
		name->SetScale((isSelf ? 0.55f : 0.45f) * textScale);
		widgets.push_back(name);
		partyHudNames.push_back(name);

		const float barH = barW * BARBACK_ASPECT;
		auto barBack = make_shared<ImageUI>(uiManager, L"BarBack", ImageUIState::Hidden);
		barBack->SetPosition(barX, barY);
		barBack->SetHoriLength(barW);
		barBack->SetVertLength(barH);
		widgets.push_back(barBack);
		partyHudBarBacks.push_back(barBack);

		const float fillW = barW * HPBAR_WIDTH_RATIO;
		const float fillH = barH * HPBAR_HEIGHT_RATIO;
		auto bar = make_shared<ImageUI>(uiManager, L"HpBar2", ImageUIState::Hidden);
		bar->SetPosition(barX + barW * HPBAR_OFFSET_X, barY + barH * HPBAR_OFFSET_Y);
		bar->SetHoriLength(fillW);
		bar->SetVertLength(fillH);
		widgets.push_back(bar);
		partyHudBars.push_back(bar);

		partyHudBarFullW[i] = fillW;
	}
}

void GameSceneUIController::RefreshPartyMemberHud()
{
	if (partyHudIcons.empty()) return;

	ClientPartyState* party = ENGINE.GetPartyState();
	const bool show = partyHudInParty && partyHudUserVisible;

	if (partyHudBack)
		partyHudBack->ChangeState(show ? ImageUIState::Visible : ImageUIState::Hidden);

	vector<const Protocol::PartyMember*> ordered;
	if (show && party && party->HasMyParty())
	{
		const Protocol::PartySnapshot& snapshot = party->GetMyParty();
		const uint32_t myId = static_cast<uint32_t>(INPUT.GetClientID());
		for (int i = 0; i < snapshot.members_size(); ++i)
			if (NetId{ snapshot.members(i).netid() }.GetId() == myId)
				ordered.push_back(&snapshot.members(i));
		for (int i = 0; i < snapshot.members_size(); ++i)
			if (NetId{ snapshot.members(i).netid() }.GetId() != myId)
				ordered.push_back(&snapshot.members(i));
	}

	for (int i = 0; i < PARTY_HUD_SLOTS; ++i)
	{
		const bool active = (i < static_cast<int>(ordered.size()));
		if (!active)
		{
			partyHudSlotIds[i] = -1;
			partyHudIcons[i]->ChangeState(ImageUIState::Hidden);
			partyHudNames[i]->SetText(L"");
			partyHudBarBacks[i]->ChangeState(ImageUIState::Hidden);
			partyHudBars[i]->ChangeState(ImageUIState::Hidden);
			continue;
		}

		const Protocol::PartyMember* member = ordered[i];
		partyHudSlotIds[i] = NetId{ member->netid() }.GetId();
		const CharacterType cls = static_cast<CharacterType>(member->charactertype());
		const wchar_t* icon =
			(cls == CharacterType::Lancer)  ? L"PartyMemLancer"  :
			(cls == CharacterType::Paladin) ? L"PartyMemPaladin" :
			                                  L"PartyMemKnight";

		partyHudIcons[i]->SetTexture(icon);
		partyHudIcons[i]->ChangeState(ImageUIState::Visible);
		partyHudNames[i]->SetText(L"ID: " + std::to_wstring(member->sessionid()));
		partyHudBarBacks[i]->ChangeState(ImageUIState::Visible);

		float pct = partyHudSelfHpPercent;
		if (i != 0)
		{
			auto hpIt = partyMemberHpPercent.find(partyHudSlotIds[i]);
			pct = (hpIt != partyMemberHpPercent.end()) ? hpIt->second : 1.0f;
		}
		partyHudBars[i]->SetHoriLength(partyHudBarFullW[i] * pct);
		partyHudBars[i]->ChangeState(ImageUIState::Visible);
	}
}

void GameSceneUIController::SetLocalCharacterType(CharacterType type)
{
	if (!statusCharImage) return;
	const wchar_t* charTex =
		(type == CharacterType::Lancer)  ? L"CharLancer"  :
		(type == CharacterType::Paladin) ? L"CharPaladin" :
		                                   L"CharKnight";
	statusCharImage->SetTexture(charTex);
}

void GameSceneUIController::InitStatWindow()
{
	statusBackImage = make_shared<ImageUI>(uiManager, L"StatusBack", ImageUIState::Hidden);
	statusBackImage->SetHoriLength(WinSize.x);
	statusBackImage->SetVertLength(WinSize.y);
	widgets.push_back(statusBackImage);

	const float leftPageCenterX = WinSize.x * 0.30f;
	const float charHeight = WinSize.y * 0.62f;
	const float charWidth  = charHeight * 0.7f;
	statusCharImage = make_shared<ImageUI>(uiManager, L"CharKnight", ImageUIState::Hidden);
	statusCharImage->SetPosition(leftPageCenterX - charWidth * 0.5f, WinSize.y * 0.14f);
	statusCharImage->SetHoriLength(charWidth);
	statusCharImage->SetVertLength(charHeight);
	widgets.push_back(statusCharImage);

	statusImage = make_shared<ImageUI>(uiManager, L"Status", ImageUIState::Hidden);
	statusImage->SetHoriLength(WinSize.x);
	statusImage->SetVertLength(WinSize.y);
	widgets.push_back(statusImage);

	const float ribbonWidth  = WinSize.x * 0.21f;
	const float ribbonHeight = ribbonWidth / 2.65f;	
	const float ribbonX = leftPageCenterX - ribbonWidth * 0.5f;
	const float ribbonY = WinSize.y * 0.78f;

	statusRibbon = make_shared<ImageUI>(uiManager, L"StatusRibbon", ImageUIState::Hidden);
	statusRibbon->SetPosition(ribbonX, ribbonY);
	statusRibbon->SetHoriLength(ribbonWidth);
	statusRibbon->SetVertLength(ribbonHeight);
	widgets.push_back(statusRibbon);

	const float ribbonTailCenter = 0.569f;
	const float ribbonTailHeight = 0.48f;
	const float arrowSizeY = ribbonHeight * ribbonTailHeight;
	const float arrowSizeX = arrowSizeY * (1440.0f / 1709.0f);
	const float arrowY = ribbonY + ribbonHeight * ribbonTailCenter - arrowSizeY * 0.5f;
	const float arrowGap = WinSize.x * 0.008f;

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

	statusStatText = make_shared<TextUI>(uiManager, L"StatText", L"VerdanaBold");
	const float statScale = WinSize.y / 1080.0f * 0.6f;
	statusStatText->SetScale(statScale);
	statusStatText->SetTextColor(Colors::White);
	statusStatText->SetPosition(WinSize.x * 0.60f, WinSize.y * 0.28f);
	statusStatText->SetText(L"");
	widgets.push_back(statusStatText);

	statusTitleText = make_shared<TextUI>(uiManager, L"StatusTitleText", L"MalgunGothic");
	const float titleScale = WinSize.y / 1080.0f * 0.4f;	
	statusTitleText->SetScale(titleScale);
	statusTitleText->SetTextColor(Colors::White);
	statusTitleText->SetPosition(ribbonX + ribbonWidth * 0.5f, ribbonY + ribbonHeight * 0.5f);
	statusTitleText->SetText(L"");
	widgets.push_back(statusTitleText);
}

void GameSceneUIController::RefreshTitleRibbon()
{
	if (!statusTitleText) return;

	uint32_t titleId = 0;
	if (ClientTitleState* titleState = ENGINE.GetTitleState())
		titleId = titleState->GetSelectedTitleId();

	const wstring name = ClientTitleState::GetDisplayName(titleId);
	statusTitleText->SetText(name);

	const float ribbonWidth  = WinSize.x * 0.21f;
	const float ribbonHeight = ribbonWidth / 2.65f;	
	const float ribbonX = WinSize.x * 0.30f - ribbonWidth * 0.5f;
	const float ribbonY = WinSize.y * 0.78f;
	const float titleScale = WinSize.y / 1080.0f * 0.4f;	

	float textW = 0.0f;
	if (auto* fd = uiManager->GetFont(L"MalgunGothic"))
		textW = XMVectorGetX(fd->font->MeasureString(name.c_str(), false)) * titleScale;

	statusTitleText->SetPosition(
		ribbonX + (ribbonWidth - textW) * 0.5f,
		ribbonY + ribbonHeight * 0.5f - 27.0f * titleScale);
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
	settingWindow = make_shared<ImageUI>(uiManager, L"CharBackground", ImageUIState::Hidden);
	settingWindow->SetPosition(0.0f, 0.0f);
	settingWindow->SetHoriLength(WinSize.x);
	settingWindow->SetVertLength(WinSize.y);
	widgets.push_back(settingWindow);
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
	joinRequestText->SetTextColor(Colors::Black);
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
			return NetId{ snapshot.members(i).netid() }.GetId() == static_cast<uint32_t>(INPUT.GetClientID());
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
		const CharacterType reqClass = static_cast<CharacterType>(active->requestercharactertype());
		const wchar_t* className =
			(reqClass == CharacterType::Lancer) ? L"Lancer" :
			(reqClass == CharacterType::Paladin) ? L"Paladin" :
													L"Knight";
		text += L"Class: " + wstring(className) + L"\n";

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
	UpdateMonsterHpBars();
	UpdateInteractPrompt();
	UpdateStatueWindow();
	UpdateBeaconWindow();
	UpdateHeroChoiceWindow();
	UpdateRespawnWindow(deltaTime);

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
			statusCharImage->ChangeState(next);
			statusImage->ChangeState(next);
			statusRibbon->ChangeState(next);
			statusArrowLeft->ChangeState(next);
			statusArrowRight->ChangeState(next);

			if (statusStatText)
				statusStatText->SetText(next == ImageUIState::Hidden ? L"" : lastStatText);

			if (next == ImageUIState::Hidden)
			{
				statusTitleText->SetText(L"");
			}
			else
			{
				NETWORK_MANAGER->SendStatUiOpenedPacket();
				lastTitleRevision = 0;
				RefreshTitleRibbon();
			}
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
			IMGUI.ShowSettingsWindow();
		}
		if (escExitButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			PostQuitMessage(0);
		}
	}

	if (settingWindow && settingWindow->GetState() != ImageUIState::Hidden)
	{
		if (IMGUI.ConsumeSettingsBack())
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			settingWindow->ChangeState(ImageUIState::Hidden);
			IMGUI.HideSettingsWindow();

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
				if (auto* network = NETWORK_MANAGER)
				{
					network->SendPartyUiClosedPacket();
				}

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
			if (ClientTitleState* titleState = ENGINE.GetTitleState())
				titleState->SelectPrevious();
		}
		if (statusArrowRight->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			if (ClientTitleState* titleState = ENGINE.GetTitleState())
				titleState->SelectNext();
		}

		if (ClientTitleState* titleState = ENGINE.GetTitleState();
			titleState && titleState->GetRevision() != lastTitleRevision)
		{
			lastTitleRevision = titleState->GetRevision();
			RefreshTitleRibbon();
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

	if (INPUT.GetKeyDown('L'))
	{
		partyHudUserVisible = !partyHudUserVisible;
		partyHudDirty = true;
	}
	{
		ClientPartyState* party = ENGINE.GetPartyState();
		const bool inParty = party && party->HasMyParty();
		if (inParty != partyHudInParty)
		{
			partyHudInParty = inParty;
			if (inParty) partyHudUserVisible = true;   
			partyHudDirty = true;
		}
		else if (inParty && party->GetRevision() != partyHudRevision)
		{
			partyHudDirty = true;
		}
		if (partyHudDirty)
		{
			partyHudRevision = inParty ? party->GetRevision() : 0;
			RefreshPartyMemberHud();
			partyHudDirty = false;
		}
	}

	const bool wantCursor =
		IMGUI.IsEnabled() ||
		opened(statusImage) || opened(escWindow)   || opened(partyBook) ||
		opened(mapImage)    || opened(keyGuide)    || opened(settingWindow) ||
		opened(joinRequestWindow) || opened(statueWindow) || opened(beaconWindow) ||
		opened(respawnWindow) || opened(heroChoiceWindow);
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

	partyHudSelfHpPercent = (maxHp > 0) ? (float)curHp / maxHp : 0.0f;
	if (!partyHudBars.empty() && partyHudBars[0])
		partyHudBars[0]->SetHoriLength(partyHudBarFullW[0] * partyHudSelfHpPercent);
}

void GameSceneUIController::HandlePartyMemberHp(int id, int cur, int max)
{
	const float pct = (max > 0) ? clamp(static_cast<float>(cur) / max, 0.0f, 1.0f) : 0.0f;
	partyMemberHpPercent[id] = pct;

	for (int i = 1; i < static_cast<int>(partyHudSlotIds.size()); ++i)
		if (partyHudSlotIds[i] == id)
			partyHudBars[i]->SetHoriLength(partyHudBarFullW[i] * pct);
}

void GameSceneUIController::HandleMonsterHp(int id, GameObject* obj, int cur, int max)
{
	if (!obj || max <= 0)
	{
		monsterHpTargets.erase(id);
		return;
	}

	auto& t = monsterHpTargets[id];
	t.obj = obj;
	t.hpPercent = clamp(static_cast<float>(cur) / max, 0.0f, 1.0f);
}

void GameSceneUIController::SetMonsterCombatState(int id, bool inCombat)
{
	monsterHpTargets[id].inCombat = inCombat;
}

void GameSceneUIController::RemoveMonsterBar(int id)
{
	monsterHpTargets.erase(id);
}

void GameSceneUIController::InitBossHpBar()
{
	constexpr float BOSSBACK_ASPECT    = 57.0f / 946.0f;
	constexpr float HPBAR_WIDTH_RATIO  = 906.0f / 946.0f;
	constexpr float HPBAR_HEIGHT_RATIO = 18.0f / 57.0f;
	constexpr float HPBAR_OFFSET_X     = 18.0f / 946.0f;
	constexpr float HPBAR_OFFSET_Y     = 19.0f / 57.0f;

	const float backW = WinSize.x * 0.55f;
	const float backH = backW * BOSSBACK_ASPECT * 0.7f;
	const float backX = (WinSize.x - backW) * 0.5f;
	const float backY = WinSize.y * 0.77f;

	bossBarBack = make_shared<ImageUI>(uiManager, L"BossBarBack", ImageUIState::Hidden);
	bossBarBack->SetPosition(backX, backY);
	bossBarBack->SetHoriLength(backW);
	bossBarBack->SetVertLength(backH);
	widgets.push_back(bossBarBack);

	bossBarFullW = backW * HPBAR_WIDTH_RATIO;
	bossBar = make_shared<ImageUI>(uiManager, L"HpBar2", ImageUIState::Hidden);
	bossBar->SetPosition(backX + backW * HPBAR_OFFSET_X, backY + backH * HPBAR_OFFSET_Y);
	bossBar->SetHoriLength(bossBarFullW);
	bossBar->SetVertLength(backH * HPBAR_HEIGHT_RATIO);
	widgets.push_back(bossBar);

	bossNameLabel = make_shared<TextUI>(uiManager, L"BossName", L"MalgunGothic");
	bossNameLabel->SetScale(WinSize.y / 1080.0f * 0.5f);
	bossNameLabel->SetPosition(backX + backW * 0.01f, backY - WinSize.y * 0.02f);
	widgets.push_back(bossNameLabel);
}

void GameSceneUIController::HandleBossHp(int cur, int max)
{
	if (max <= 0) return;   

	bossHpPercent = clamp(static_cast<float>(cur) / max, 0.0f, 1.0f);
	if (bossBar)
		bossBar->SetHoriLength(bossBarFullW * bossHpPercent);
}

void GameSceneUIController::SetBossCombatState(bool inCombat, MonsterType bossType)
{
	bossInCombat = inCombat;
	const ImageUIState state = inCombat ? ImageUIState::Visible : ImageUIState::Hidden;
	if (bossBarBack) bossBarBack->ChangeState(state);
	if (bossBar)     bossBar->ChangeState(state);

	if (bossNameLabel)
	{
		const wchar_t* name = L"";
		switch (bossType)
		{
		case MonsterType::BigDemonWarrior: name = L"검은 가시의 분쇄자, 그라툼"; break;
		case MonsterType::Tank:            name = L"무쇠뿔의 거수, 브룬타크";   break;
		case MonsterType::Boss:            name = L"왕좌의 흑기사, 벨카리온";   break;
		default: break;
		}
		bossNameLabel->SetText(inCombat ? name : L"");

		static const XMVECTORF32 bloodRed = { 0.62f, 0.04f, 0.05f, 1.0f };
		bossNameLabel->SetTextColor(bossType == MonsterType::Boss ? bloodRed : Colors::AntiqueWhite);
	}
}

void GameSceneUIController::RemoveBossHpBar()
{
	bossInCombat = false;
	if (bossBarBack) bossBarBack->ChangeState(ImageUIState::Hidden);
	if (bossBar)     bossBar->ChangeState(ImageUIState::Hidden);
	if (bossNameLabel) bossNameLabel->SetText(L"");
}

void GameSceneUIController::UpdateMonsterHpBars()
{
	constexpr float BARBACK_ASPECT     = 39.0f / 785.0f;
	constexpr float HPBAR_WIDTH_RATIO  = 692.0f / 785.0f;
	constexpr float HPBAR_HEIGHT_RATIO = 18.0f / 39.0f;
	constexpr float HPBAR_OFFSET_X     = 49.0f / 785.0f;
	constexpr float HPBAR_OFFSET_Y     = 11.0f / 39.0f;
	constexpr float BAR_WORLD_WIDTH    = 1.0f;  
	constexpr float BAR_HEIGHT_SCALE   = 2.2f;  

	Scene* scene = SCENE_MANAGER->GetCurrentScene();
	Camera* camera = scene ? scene->GetCamera() : nullptr;

	int used = 0;
	if (camera)
	{
		const XMMATRIX viewProj = camera->GetViewMatrix() * camera->GetProjectionMatrix();
		const XMFLOAT3 camRight = camera->GetRight();

		auto project = [&](const XMFLOAT3& p, float& sx, float& sy) -> bool {
			const XMVECTOR c = XMVector4Transform(XMVectorSetW(XMLoadFloat3(&p), 1.0f), viewProj);
			const float w = XMVectorGetW(c);
			if (w <= 0.0001f) return false;
			sx = (XMVectorGetX(c) / w * 0.5f + 0.5f) * WinSize.x;
			sy = (1.0f - (XMVectorGetY(c) / w * 0.5f + 0.5f)) * WinSize.y;
			return true;
		};

		for (const auto& [id, target] : monsterHpTargets)
		{
			if (used >= MAX_MONSTER_HP_BARS) break;
			if (!target.obj || target.obj->GetId() == -1) continue;
			if (!target.inCombat) continue;

			auto* tf = target.obj->GetComponent<Transform>();
			if (!tf) continue;

			const XMFLOAT3& pos = tf->GetPosition();
			const BoundingOrientedBox& box = target.obj->GetWorldBoundingBox();
			const XMFLOAT3 head{ pos.x, box.Center.y + box.Extents.y, pos.z };

			const XMFLOAT3 headRight{
				head.x + camRight.x * BAR_WORLD_WIDTH,
				head.y + camRight.y * BAR_WORLD_WIDTH,
				head.z + camRight.z * BAR_WORLD_WIDTH };

			float cx, cy, rx, ry;
			if (!project(head, cx, cy)) continue;                     
			if (!project(headRight, rx, ry)) continue;
			if (cx < 0.0f || cx > WinSize.x || cy < 0.0f || cy > WinSize.y) continue; 

			const float barW = fabsf(rx - cx);
			const float barH = barW * BARBACK_ASPECT * BAR_HEIGHT_SCALE;
			const float gap  = barH * 0.5f;

			const float backX = cx - barW * 0.5f;
			const float backY = cy - barH - gap;

			monsterBarBacks[used]->SetPosition(backX, backY);
			monsterBarBacks[used]->SetHoriLength(barW);
			monsterBarBacks[used]->SetVertLength(barH);
			monsterBarBacks[used]->ChangeState(ImageUIState::Visible);

			monsterBars[used]->SetPosition(backX + barW * HPBAR_OFFSET_X, backY + barH * HPBAR_OFFSET_Y);
			monsterBars[used]->SetHoriLength(barW * HPBAR_WIDTH_RATIO * target.hpPercent);
			monsterBars[used]->SetVertLength(barH * HPBAR_HEIGHT_RATIO);
			monsterBars[used]->ChangeState(ImageUIState::Visible);

			++used;
		}
	}

	for (int i = used; i < MAX_MONSTER_HP_BARS; ++i)
	{
		monsterBarBacks[i]->ChangeState(ImageUIState::Hidden);
		monsterBars[i]->ChangeState(ImageUIState::Hidden);
	}
}

void GameSceneUIController::InitInteractPrompt()
{
	interactCircle = make_shared<ImageUI>(uiManager, L"MagicCircle", ImageUIState::Hidden);
	widgets.push_back(interactCircle);

	interactScale = 0.7f;   

	interactKeyText = make_shared<TextUI>(uiManager, L"InteractKey", L"VerdanaBold");
	interactKeyText->SetText(L"");
	interactKeyText->SetScale(WinSize.y / 1080.0f * 0.85f * interactScale);
	widgets.push_back(interactKeyText);

	interactLabelText = make_shared<TextUI>(uiManager, L"InteractLabel", L"VerdanaBold");
	interactLabelText->SetText(L"");
	interactLabelText->SetScale(WinSize.y / 1080.0f * 0.6f * interactScale);
	widgets.push_back(interactLabelText);
}

void GameSceneUIController::SetInteractPrompt(bool active, const XMFLOAT3& worldAnchor)
{
	interactActive = active;
	interactWorldAnchor = worldAnchor;
}

void GameSceneUIController::UpdateInteractPrompt()
{
	if (!interactCircle) return;

	const bool windowOpen =
		(statueWindow && statueWindow->GetState() != ImageUIState::Hidden) ||
		(beaconWindow && beaconWindow->GetState() != ImageUIState::Hidden);

	if (!interactActive || windowOpen || !IsMyPartyLeader())
	{
		interactCircle->ChangeState(ImageUIState::Hidden);
		interactKeyText->SetText(L"");
		interactLabelText->SetText(L"");
		return;
	}

	Scene* scene = SCENE_MANAGER->GetCurrentScene();
	Camera* camera = scene ? scene->GetCamera() : nullptr;
	if (!camera)
	{
		interactCircle->ChangeState(ImageUIState::Hidden);
		interactKeyText->SetText(L"");
		interactLabelText->SetText(L"");
		return;
	}

	const XMMATRIX viewProj = camera->GetViewMatrix() * camera->GetProjectionMatrix();
	const XMVECTOR c = XMVector4Transform(XMVectorSetW(XMLoadFloat3(&interactWorldAnchor), 1.0f), viewProj);
	const float w = XMVectorGetW(c);
	if (w <= 0.0001f)   
	{
		interactCircle->ChangeState(ImageUIState::Hidden);
		interactKeyText->SetText(L"");
		interactLabelText->SetText(L"");
		return;
	}

	const float sx = (XMVectorGetX(c) / w * 0.5f + 0.5f) * WinSize.x;
	const float sy = (1.0f - (XMVectorGetY(c) / w * 0.5f + 0.5f)) * WinSize.y;

	const float circleSize = WinSize.y * 0.085f * interactScale;
	interactCircle->SetPosition(sx - circleSize * 0.5f, sy - circleSize * 0.5f);
	interactCircle->SetHoriLength(circleSize);
	interactCircle->SetVertLength(circleSize);
	interactCircle->ChangeState(ImageUIState::Visible);

	interactKeyText->SetText(L"F");
	interactKeyText->SetPosition(sx - circleSize * 0.21f, sy - circleSize * 0.27f);
	interactLabelText->SetText(L"Interact");
	interactLabelText->SetPosition(sx + circleSize * 0.62f, sy - circleSize * 0.22f);
}

void GameSceneUIController::InitStatueWindow()
{
	if (sceneType != SceneType::Plaza) return;   

	const float winSize = WinSize.y * 0.5f;
	const float winX = (WinSize.x - winSize) * 0.5f;
	const float winY = (WinSize.y - winSize) * 0.5f;

	statueWindow = make_shared<ImageUI>(uiManager, L"StatueInteractWindow", ImageUIState::Hidden);
	statueWindow->SetPosition(winX, winY);
	statueWindow->SetHoriLength(winSize);
	statueWindow->SetVertLength(winSize);
	widgets.push_back(statueWindow);

	const float btnW = winSize * 0.30f;
	const float btnH = btnW / 3.879f;          
	const float btnGap = winSize * 0.06f;
	const float btnY = winY + winSize * 0.65f;
	const float btnLeftX = winX + (winSize - btnW * 2.0f - btnGap) * 0.5f;

	statueOkButton = make_shared<ImageUI>(uiManager, L"OK", ImageUIState::Hidden);
	statueOkButton->SetPosition(btnLeftX, btnY);
	statueOkButton->SetHoriLength(btnW);
	statueOkButton->SetVertLength(btnH);
	statueOkButton->SetHoverScale(1.1f);
	widgets.push_back(statueOkButton);

	statueCancelButton = make_shared<ImageUI>(uiManager, L"CANCEL", ImageUIState::Hidden);
	statueCancelButton->SetPosition(btnLeftX + btnW + btnGap, btnY);
	statueCancelButton->SetHoriLength(btnW);
	statueCancelButton->SetVertLength(btnH);
	statueCancelButton->SetHoverScale(1.1f);
	widgets.push_back(statueCancelButton);
}

void GameSceneUIController::UpdateStatueWindow()
{
	if (!statueWindow) return;

	auto setWindow = [&](ImageUIState s) {
		statueWindow->ChangeState(s);
		statueOkButton->ChangeState(s);
		statueCancelButton->ChangeState(s);
	};

	const bool open = statueWindow->GetState() != ImageUIState::Hidden;

	if (!interactActive || !IsMyPartyLeader())
	{
		if (open) setWindow(ImageUIState::Hidden);
		return;
	}

	if (!open && INPUT.GetKeyDown('F'))
	{
		setWindow(ImageUIState::Visible);
		return;
	}

	if (!open) return;

	statueOkButton->SetHovered(statueOkButton->IsMouseInside());
	statueCancelButton->SetHovered(statueCancelButton->IsMouseInside());

	if (statueOkButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		setWindow(ImageUIState::Hidden);

		if (auto* fade = uiManager->GetScreenFade())
		{
			fade->SetOnFadedOut([]() {
				auto& transition = ENGINE.GetWorldTransitionController();
				const uint32_t requestId = transition.CreateRequestId();
				if (transition.BeginRequest(requestId))
				{
					if (!NETWORK_MANAGER->SendWorldTransitionRequestPacket(requestId))
						transition.Reset();
				}
			});
			fade->FadeOut(1.0f);
		}
	}
	else if (statueCancelButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		setWindow(ImageUIState::Hidden);
	}
}

void GameSceneUIController::InitBeaconWindow()
{
	if (sceneType != SceneType::Village && sceneType != SceneType::Castle) return;

	const float winSize = WinSize.y * 0.5f;
	const float winX = (WinSize.x - winSize) * 0.5f;
	const float winY = (WinSize.y - winSize) * 0.5f;

	beaconWindow = make_shared<ImageUI>(uiManager, L"BeaconInteractWindow", ImageUIState::Hidden);
	beaconWindow->SetPosition(winX, winY);
	beaconWindow->SetHoriLength(winSize);
	beaconWindow->SetVertLength(winSize);
	widgets.push_back(beaconWindow);

	const float btnW = winSize * 0.30f;
	const float btnH = btnW / 3.879f;
	const float btnGap = winSize * 0.06f;
	const float btnY = winY + winSize * 0.65f;
	const float btnLeftX = winX + (winSize - btnW * 2.0f - btnGap) * 0.5f;

	beaconOkButton = make_shared<ImageUI>(uiManager, L"OK", ImageUIState::Hidden);
	beaconOkButton->SetPosition(btnLeftX, btnY);
	beaconOkButton->SetHoriLength(btnW);
	beaconOkButton->SetVertLength(btnH);
	beaconOkButton->SetHoverScale(1.1f);
	widgets.push_back(beaconOkButton);

	beaconCancelButton = make_shared<ImageUI>(uiManager, L"CANCEL", ImageUIState::Hidden);
	beaconCancelButton->SetPosition(btnLeftX + btnW + btnGap, btnY);
	beaconCancelButton->SetHoriLength(btnW);
	beaconCancelButton->SetVertLength(btnH);
	beaconCancelButton->SetHoverScale(1.1f);
	widgets.push_back(beaconCancelButton);
}

void GameSceneUIController::UpdateBeaconWindow()
{
	if (!beaconWindow) return;

	auto setWindow = [&](ImageUIState s) {
		beaconWindow->ChangeState(s);
		beaconOkButton->ChangeState(s);
		beaconCancelButton->ChangeState(s);
	};

	const bool open = beaconWindow->GetState() != ImageUIState::Hidden;

	if (!interactActive || !IsMyPartyLeader())
	{
		if (open) setWindow(ImageUIState::Hidden);
		return;
	}

	if (!open && INPUT.GetKeyDown('F'))
	{
		setWindow(ImageUIState::Visible);
		return;
	}

	if (!open) return;

	beaconOkButton->SetHovered(beaconOkButton->IsMouseInside());
	beaconCancelButton->SetHovered(beaconCancelButton->IsMouseInside());

	if (beaconOkButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		setWindow(ImageUIState::Hidden);
		beaconConfirmed = true;
	}
	else if (beaconCancelButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		setWindow(ImageUIState::Hidden);
	}
}

bool GameSceneUIController::ConsumeBeaconConfirmed()
{
	const bool v = beaconConfirmed;
	beaconConfirmed = false;
	return v;
}

void GameSceneUIController::InitHeroChoiceWindow()
{
	if (sceneType != SceneType::Final) return;

	const float winW = WinSize.x * 0.5f;
	const float winH = winW / 2.6f;
	const float winX = (WinSize.x - winW) * 0.5f;
	const float winY = (WinSize.y - winH) * 0.5f;

	heroChoiceWindow = make_shared<ImageUI>(uiManager, L"WITH", ImageUIState::Hidden);
	heroChoiceWindow->SetPosition(winX, winY);
	heroChoiceWindow->SetHoriLength(winW);
	heroChoiceWindow->SetVertLength(winH);
	widgets.push_back(heroChoiceWindow);

	const float btnW = winW * 0.18f;
	const float btnH = btnW / 3.0f;
	const float btnY = winY + winH * 0.78f;

	heroMeButton = make_shared<ImageUI>(uiManager, L"ME", ImageUIState::Hidden);
	heroMeButton->SetPosition(winX + winW * 0.28f - btnW * 0.5f, btnY);
	heroMeButton->SetHoriLength(btnW);
	heroMeButton->SetVertLength(btnH);
	heroMeButton->SetHoverScale(1.1f);
	widgets.push_back(heroMeButton);

	heroWeButton = make_shared<ImageUI>(uiManager, L"WE", ImageUIState::Hidden);
	heroWeButton->SetPosition(winX + winW * 0.72f - btnW * 0.5f, btnY);
	heroWeButton->SetHoriLength(btnW);
	heroWeButton->SetVertLength(btnH);
	heroWeButton->SetHoverScale(1.1f);
	widgets.push_back(heroWeButton);
}

void GameSceneUIController::UpdateHeroChoiceWindow()
{
	if (!heroChoiceWindow) return;
	if (heroChoiceWindow->GetState() == ImageUIState::Hidden) return;

	heroMeButton->SetHovered(heroMeButton->IsMouseInside());
	heroWeButton->SetHovered(heroWeButton->IsMouseInside());

	const bool meClick = heroMeButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT);
	const bool weClick = heroWeButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT);

	if (meClick || weClick)
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		if (auto* network = NETWORK_MANAGER)
			network->SendFinalClearChoiceSubmit(heroChoiceVoteId, meClick);	
		HideHeroChoice();
	}
}

void GameSceneUIController::ShowHeroChoice(uint64_t voteId)
{
	if (!heroChoiceWindow) return;

	heroChoiceVoteId = voteId;
	heroChoiceWindow->ChangeState(ImageUIState::Visible);
	heroMeButton->ChangeState(ImageUIState::Visible);
	heroWeButton->ChangeState(ImageUIState::Visible);
}

void GameSceneUIController::HideHeroChoice()
{
	if (!heroChoiceWindow) return;

	heroChoiceWindow->ChangeState(ImageUIState::Hidden);
	heroMeButton->ChangeState(ImageUIState::Hidden);
	heroWeButton->ChangeState(ImageUIState::Hidden);
}

void GameSceneUIController::InitRespawnWindow()
{
	if (sceneType != SceneType::Village && sceneType != SceneType::Castle && sceneType != SceneType::Final)
		return;

	const float winSize = WinSize.y * 0.5f;
	const float winX = (WinSize.x - winSize) * 0.5f;
	const float winY = (WinSize.y - winSize) * 0.5f;
	const float textScale = WinSize.y / 1080.0f;

	respawnWindow = make_shared<ImageUI>(uiManager, L"RespawnWindow", ImageUIState::Hidden);
	respawnWindow->SetPosition(winX, winY);
	respawnWindow->SetHoriLength(winSize);
	respawnWindow->SetVertLength(winSize);
	widgets.push_back(respawnWindow);

	respawnCountText = make_shared<TextUI>(uiManager, L"RespawnCount", L"MalgunGothic");
	respawnCountText->SetPosition(winX + winSize * 0.20f, winY + winSize * 0.56f);
	respawnCountText->SetScale(0.5f * textScale);
	respawnCountText->SetTextColor(Colors::Red);
	widgets.push_back(respawnCountText);

	const float btnW = winSize * 0.30f;
	const float btnH = btnW / 3.879f;
	const float btnX = winX + (winSize - btnW) * 0.5f;
	const float btnY = winY + winSize * 0.72f;

	respawnOkButton = make_shared<ImageUI>(uiManager, L"OK", ImageUIState::Hidden);
	respawnOkButton->SetPosition(btnX, btnY);
	respawnOkButton->SetHoriLength(btnW);
	respawnOkButton->SetVertLength(btnH);
	respawnOkButton->SetHoverScale(1.1f);
	widgets.push_back(respawnOkButton);
}

void GameSceneUIController::OnLocalPlayerDied()
{
	if (!respawnWindow || respawnActive) return;

	respawnActive = true;
	respawnTimer = RESPAWN_SECONDS;
	respawnWindow->ChangeState(ImageUIState::Visible);
	respawnOkButton->ChangeState(ImageUIState::Visible);
}

void GameSceneUIController::OnLocalPlayerRevived()
{
	if (!respawnWindow) return;

	respawnActive = false;
	respawnWindow->ChangeState(ImageUIState::Hidden);
	respawnOkButton->ChangeState(ImageUIState::Hidden);
	respawnCountText->SetText(L"");
}

void GameSceneUIController::UpdateRespawnWindow(float deltaTime)
{
	if (!respawnWindow) return;

	if (Scene* scene = SCENE_MANAGER->GetCurrentScene())
	{
		if (MainCharacter* me = scene->GetMyPlayer())
		{
			auto* am = me->GetComponent<AnimationMachine>();
			auto* anim = me->GetComponent<Animator>();
			const bool dieFrozen = am && anim &&
				am->GetCurrentCategory() == AnimCategory::Die &&
				anim->GetAnimationProgress() >= 0.99f;

			if (dieFrozen && !localDeadHandled)
			{
				OnLocalPlayerDied();
				localDeadHandled = true;
			}
			else if (am && am->GetCurrentCategory() != AnimCategory::Die && localDeadHandled)
			{
				OnLocalPlayerRevived();
				localDeadHandled = false;
			}
		}
	}

	if (!respawnActive) return;

	respawnTimer -= deltaTime;
	if (respawnTimer < 0.0f) respawnTimer = 0.0f;

	const int sec = static_cast<int>(ceil(respawnTimer));
	const wstring text = to_wstring(sec) + L"초 뒤에 자동으로 부활합니다";
	respawnCountText->SetText(text);

	if (auto* fd = uiManager->GetFont(L"MalgunGothic"))
	{
		const float winSize = WinSize.y * 0.5f;
		const float winX = (WinSize.x - winSize) * 0.5f;
		const float winY = (WinSize.y - winSize) * 0.5f;
		const float scale = 0.5f * (WinSize.y / 1080.0f);
		const float textW = XMVectorGetX(fd->font->MeasureString(text.c_str(), false)) * scale;
		respawnCountText->SetPosition(winX + (winSize - textW) * 0.5f, winY + winSize * 0.56f);
	}

	respawnOkButton->SetHovered(respawnOkButton->IsMouseInside());
	if (respawnOkButton->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		RequestRespawn();  
		return;
	}

	if (respawnTimer <= 0.0f)
		RequestRespawn();  
}

void GameSceneUIController::RequestRespawn()
{
	SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");

	if (NETWORK_MANAGER->SendRespawnRequestPacket())
	{
		respawnActive = false;
		respawnWindow->ChangeState(ImageUIState::Hidden);
		respawnOkButton->ChangeState(ImageUIState::Hidden);
		respawnCountText->SetText(L"");
	}
}

void GameSceneUIController::HideHudForCinematic()
{
	auto hide = [](const shared_ptr<ImageUI>& w) { if (w) w->ChangeState(ImageUIState::Hidden); };

	hide(localCharBarsBack);
	hide(localCharHpBar);
	hide(localCharStaminaBar);
	hide(localCharPotion);
	if (localCharPotionCount) localCharPotionCount->SetText(L"");
	hide(localCharDeathCount);
	if (localCharDeathCountText) localCharDeathCountText->SetText(L"");

	hide(statusBackImage); hide(statusCharImage); hide(statusImage);
	hide(statusRibbon); hide(statusArrowLeft); hide(statusArrowRight);

	hide(escWindow); hide(escContinueButton); hide(escOptionsButton); hide(escExitButton);
	hide(settingWindow);
	hide(keyGuide);

	hide(mapBackImage); hide(mapImage);

	partyHudUserVisible = false;
	RefreshPartyMemberHud();
}

void GameSceneUIController::HandleStatImageChange(int curHp, int maxHp, int curStamina, int maxStamina, int power, double aSpeed, int defense, double mSpeed)
{
	wstring text =
		L"Current Hp: " + to_wstring(curHp) + L"\n" + L"\n" +
		L"Max Hp: " + to_wstring(maxHp) + L"\n" + L"\n" +
		L"Current Stamina: " + to_wstring(curStamina) + L"\n" + L"\n" +
		L"Max Stamina: " + to_wstring(maxStamina) + L"\n" + L"\n" +
		L"Power: " + to_wstring(power) + L"\n" + L"\n" +
		L"Attack Speed: " + to_wstring(aSpeed) + L"\n" + L"\n" +
		L"Defense: " + to_wstring(defense) + L"\n" + L"\n" +
		L"Move Speed: " + to_wstring(mSpeed) + L"\n";

	lastStatText = text;

	if (statusStatText && IsStatWindowOn())
		statusStatText->SetText(text);
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
