#include "pch.h"
#include "SelectSceneUIController.h"
#include "ImageUI.h"

void SelectSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	float charWidth = 0.0f;
	float charHeight = 0.0f;

	InitBackground();
	InitCharImages(charWidth, charHeight);
	InitHoverOverlay(charWidth, charHeight);
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

void SelectSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	bool anyHovered = false;
	for (int i = 0; i < 3; i++)
	{
		if (!charImages[i]) continue;
		if (charImages[i]->IsMouseInside())
		{
			hoverOverlay->SetPosition(charImages[i]->GetPosX() + hoverOffsetX,
									  charImages[i]->GetPosY() + hoverOffsetY);
			if (hoverOverlay->GetState() == ImageUIState::Hidden)
				hoverOverlay->ChangeState(ImageUIState::FadingIn);
			anyHovered = true;
		}
	}

	if (!anyHovered && hoverOverlay->GetState() != ImageUIState::Hidden)
		hoverOverlay->ChangeState(ImageUIState::Hidden);
}

