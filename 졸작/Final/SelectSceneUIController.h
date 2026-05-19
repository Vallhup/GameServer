#pragma once
#include "UIController.h"

class ImageUI;

class SelectSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;

private:
	void InitBackground();
	void InitCharImages(float& outCharWidth, float& outCharHeight);
	void InitHoverOverlay(float charWidth, float charHeight);
	void InitSelectWindow();

	shared_ptr<ImageUI> background;
	shared_ptr<ImageUI> charImages[3];
	shared_ptr<ImageUI> hoverOverlay;

	shared_ptr<ImageUI> selectWindow;
	shared_ptr<ImageUI> okButton;
	shared_ptr<ImageUI> cancelButton;

	float hoverOffsetX = 0.0f;
	float hoverOffsetY = 0.0f;

	int selectedChar = -1;
	bool windowOpen = false;
};

