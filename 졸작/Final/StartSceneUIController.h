#pragma once
#include "UIController.h"

class ImageUI;

class StartSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

private:
	shared_ptr<ImageUI> mainImage;
	shared_ptr<ImageUI> pabImage;
	shared_ptr<ImageUI> loginImage;
	shared_ptr<ImageUI> exitImage;
};

