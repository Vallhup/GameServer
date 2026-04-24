#include "pch.h"
#include "SelectSceneUIController.h"
#include "ImageUI.h"

void SelectSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	background = make_shared<ImageUI>(L"CharBackground", ImageUIState::Visible);
	background->Init(uiManager);
	background->SetHoriLength(WinSize.x);
	background->SetVertLength(WinSize.y);

	const wstring charNames[3] = { L"CharA", L"CharB", L"CharC" };

	float sectionWidth = WinSize.x / 3.0f;
	float charWidth = sectionWidth * 0.8f;
	float charHeight = WinSize.y * 0.7f;
	float padX = (sectionWidth - charWidth) / 2.0f;
	float posY = (WinSize.y - charHeight) / 2.0f;

	for (int i = 0; i < 3; i++)
	{
		charImages[i] = make_shared<ImageUI>(charNames[i], ImageUIState::Visible);
		charImages[i]->Init(uiManager);
		charImages[i]->SetPosition(sectionWidth * i + padX, posY);
		charImages[i]->SetHoriLength(charWidth);
		charImages[i]->SetVertLength(charHeight);
	}

	hoverOverlay = make_shared<ImageUI>(L"CharHover", ImageUIState::Hidden);
	hoverOverlay->Init(uiManager);
	hoverOverlay->SetHoriLength(charWidth);
	hoverOverlay->SetVertLength(charHeight);
}

void SelectSceneUIController::Update(float deltaTime)
{
	if (background) background->Update(deltaTime);

	bool anyHovered = false;
	for (int i = 0; i < 3; i++)
	{
		if (!charImages[i]) continue;
		charImages[i]->Update(deltaTime);

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

	if (hoverOverlay) hoverOverlay->Update(deltaTime);
}

void SelectSceneUIController::Render(SpriteBatch* batch)
{
	/*if (background) background->Render(batch);

	for (auto& img : charImages)
		if (img) img->Render(batch);

	if (hoverOverlay) hoverOverlay->Render(batch);*/
}
