#pragma once
#include "UIController.h"

class ImageUI;
class TextUI;

class GameSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void HandleStatBarChange(int curHp, int maxHp, int curStamina, int maxStamina);
	void HandleStatImageChange(
		int curHp, int maxHp, int curStamina, int maxStamina, 
		int power, double aSpeed, int defense, int mSpeed);
	bool IsStatWindowOn() const;

private:
	shared_ptr<ImageUI> statusImage;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;

	shared_ptr<TextUI>  tempStatusText;

	shared_ptr<ImageUI> charHPBarBack;
	shared_ptr<ImageUI> charHPBar;
};

