#include "pch.h"
#include "SelectSceneUIController.h"
#include "ImageUI.h"
#include "Input.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "SoundManager.h"

void SelectSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	float charWidth = 0.0f;
	float charHeight = 0.0f;

	InitBackground();
	InitCharImages(charWidth, charHeight);
	InitHoverOverlay(charWidth, charHeight);
	InitSelectWindow();
}

void SelectSceneUIController::InitBackground()
{
	background = make_shared<ImageUI>(uiManager, L"CharBackground", ImageUIState::Visible);
	background->SetHoriLength(WinSize.x);
	background->SetVertLength(WinSize.y);
	widgets.push_back(background);
}

void SelectSceneUIController::InitCharImages(float& outCharWidth, float& outCharHeight)
{
	const wstring charNames[3] = { L"CharKnight", L"CharLancer", L"CharPaladin" };

	float sectionWidth = WinSize.x / 3.0f;
	float charWidth = sectionWidth * 0.8f;
	float charHeight = WinSize.y * 0.7f;
	float padX = (sectionWidth - charWidth) / 2.0f;
	float posY = (WinSize.y - charHeight) / 2.0f;

	for (int i = 0; i < 3; i++)
	{
		charImages[i] = make_shared<ImageUI>(uiManager, charNames[i], ImageUIState::Visible);
		charImages[i]->SetPosition(sectionWidth * i + padX, posY);
		charImages[i]->SetHoriLength(charWidth);
		charImages[i]->SetVertLength(charHeight);
		widgets.push_back(charImages[i]);
	}

	outCharWidth = charWidth;
	outCharHeight = charHeight;
}

void SelectSceneUIController::InitHoverOverlay(float charWidth, float charHeight)
{
	constexpr float texW = 512.0f, texH = 756.0f;
	constexpr float outL = 15.0f,  outR = 499.0f;   
	constexpr float outT = 28.0f,  outB = 729.0f;   
	constexpr float outW = outR - outL;             
	constexpr float outH = outB - outT;             

	const float drawW = charWidth  * (texW / outW);
	const float drawH = charHeight * (texH / outH);

	hoverOffsetX = -(outL / texW) * drawW;
	hoverOffsetY = -(outT / texH) * drawH;

	hoverOverlay = make_shared<ImageUI>(uiManager, L"CharHover", ImageUIState::Hidden);
	hoverOverlay->SetHoriLength(drawW);
	hoverOverlay->SetVertLength(drawH);
	widgets.push_back(hoverOverlay);
}

void SelectSceneUIController::InitSelectWindow()
{
	const float windowW = WinSize.x * 0.35f;
	const float windowH = windowW / 1.75f;
	const float windowX = (WinSize.x - windowW) / 2.0f;
	const float windowY = (WinSize.y - windowH) / 2.0f;

	selectWindow = make_shared<ImageUI>(uiManager, L"SelectWindow", ImageUIState::Hidden);
	selectWindow->SetPosition(windowX, windowY);
	selectWindow->SetHoriLength(windowW);
	selectWindow->SetVertLength(windowH);
	selectWindow->SetFadeDuration(0.8f);
	widgets.push_back(selectWindow);

	const float btnW = windowW * 0.38f;
	const float btnH = btnW / 3.879f;
	const float btnY = windowY + windowH * 0.60f;

	okButton = make_shared<ImageUI>(uiManager, L"OK", ImageUIState::Hidden);
	okButton->SetPosition(windowX + windowW * 0.08f, btnY);
	okButton->SetHoriLength(btnW);
	okButton->SetVertLength(btnH);
	okButton->SetFadeDuration(0.8f);
	okButton->SetHoverScale(1.1f);
	widgets.push_back(okButton);

	cancelButton = make_shared<ImageUI>(uiManager, L"CANCEL", ImageUIState::Hidden);
	cancelButton->SetPosition(windowX + windowW * 0.54f, btnY);
	cancelButton->SetHoriLength(btnW);
	cancelButton->SetVertLength(btnH);
	cancelButton->SetFadeDuration(0.8f);
	cancelButton->SetHoverScale(1.1f);
	widgets.push_back(cancelButton);
}

void SelectSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	if (windowOpen)
	{
		okButton->SetHovered(okButton->IsMouseInside());
		cancelButton->SetHovered(cancelButton->IsMouseInside());

		if (INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			if (cancelButton->IsMouseInside())
			{
				SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
				selectWindow->ChangeState(ImageUIState::Hidden);
				okButton->ChangeState(ImageUIState::Hidden);
				cancelButton->ChangeState(ImageUIState::Hidden);
				hoverOverlay->ChangeState(ImageUIState::Hidden);
				windowOpen = false;
				selectedChar = -1;
			}
			else if (okButton->IsMouseInside() && selectedChar >= 0)
			{
				SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
				static constexpr CharacterId charIds[3] = { CharacterId::Knight, CharacterId::Lancer, CharacterId::Paladin };
				NETWORK_MANAGER->SendCharacterSelectPacket(charIds[selectedChar]);
				SCENE_MANAGER->RequestLoadingScene(SceneType::Plaza);
			}
		}
		return;
	}

	bool anyHovered = false;
	for (int i = 0; i < 3; i++)
	{
		if (!charImages[i]) continue;
		if (!charImages[i]->IsMouseInside()) continue;

		hoverOverlay->SetPosition(charImages[i]->GetPosX() + hoverOffsetX,
								  charImages[i]->GetPosY() + hoverOffsetY);
		if (hoverOverlay->GetState() == ImageUIState::Hidden)
			hoverOverlay->ChangeState(ImageUIState::FadingIn);
		anyHovered = true;

		if (INPUT.GetMouseButtonDown(MouseButton::LEFT))
		{
			SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
			selectedChar = i;
			windowOpen = true;
			hoverOverlay->ChangeState(ImageUIState::Visible);
			selectWindow->ChangeState(ImageUIState::FadingIn);
			okButton->ChangeState(ImageUIState::FadingIn);
			cancelButton->ChangeState(ImageUIState::FadingIn);
		}
	}

	if (!anyHovered && hoverOverlay->GetState() != ImageUIState::Hidden)
		hoverOverlay->ChangeState(ImageUIState::Hidden);
}

