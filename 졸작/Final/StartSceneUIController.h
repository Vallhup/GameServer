#pragma once
#include "UIController.h"

class ImageUI;
class UIComponent;

class StartSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

private:
	void InitMainImage();
	void InitPressAnyButton();
	void InitMenuButtons();

	vector<shared_ptr<UIComponent>> widgets;

	shared_ptr<ImageUI> mainImage;
	shared_ptr<ImageUI> pabImage;
	shared_ptr<ImageUI> loginImage;
	shared_ptr<ImageUI> exitImage;
};

