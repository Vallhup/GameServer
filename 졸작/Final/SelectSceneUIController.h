#pragma once
#include "UIController.h"

class ImageUI;

class SelectSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

private:
	shared_ptr<ImageUI> backgroundImage;
};

