#pragma once
#include "UIController.h"

class ImageUI;

class GameSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void HandleStatChange(int hp, int stamina);

private:
	shared_ptr<ImageUI> statusImage;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;
};

