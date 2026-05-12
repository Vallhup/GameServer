#pragma once
#include "UIController.h"

class ImageUI;
class UIComponent;

class SelectSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

private:
	void InitBackground();
	void InitCharImages(float& outCharWidth, float& outCharHeight);
	void InitHoverOverlay(float charWidth, float charHeight);

	vector<shared_ptr<UIComponent>> widgets;

	shared_ptr<ImageUI> background;
	shared_ptr<ImageUI> charImages[3];
	shared_ptr<ImageUI> hoverOverlay;
};

