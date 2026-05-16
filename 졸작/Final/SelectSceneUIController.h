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

	shared_ptr<ImageUI> background;
	shared_ptr<ImageUI> charImages[3];
	shared_ptr<ImageUI> hoverOverlay;

	float hoverOffsetX = 0.0f;
	float hoverOffsetY = 0.0f;
};

