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
	const wstring charNames[3] = { L"CharA", L"CharB", L"CharC" };

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
	hoverOverlay = make_shared<ImageUI>(uiManager, L"CharHover", ImageUIState::Hidden);
	hoverOverlay->SetHoriLength(charWidth);
	hoverOverlay->SetVertLength(charHeight);
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
			hoverOverlay->SetPosition(charImages[i]->GetPosX(), charImages[i]->GetPosY());
			if (hoverOverlay->GetState() == ImageUIState::Hidden)
				hoverOverlay->ChangeState(ImageUIState::Pulsing);
			anyHovered = true;
		}
	}

	if (!anyHovered && hoverOverlay->GetState() != ImageUIState::Hidden)
		hoverOverlay->ChangeState(ImageUIState::Hidden);
}

