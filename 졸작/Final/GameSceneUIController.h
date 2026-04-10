#pragma once
#include "UIController.h"
#include "SceneManager.h"

class ImageUI;
class TextUI;

class GameSceneUIController : public UIController
{
public:
	GameSceneUIController(SceneType type);

	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void HandleStatBarChange(int curHp, int maxHp, int curStamina, int maxStamina);
	void HandleStatImageChange(
		int curHp, int maxHp, int curStamina, int maxStamina,
		int power, double aSpeed, int defense, double mSpeed);
	bool IsStatWindowOn() const;

	void ShowMapName();

private:
	SceneType sceneType;

	shared_ptr<ImageUI> statusImage;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;

	shared_ptr<TextUI>  tempStatusText;

	shared_ptr<ImageUI> charHPBarBack;
	shared_ptr<ImageUI> charHPBar;

	shared_ptr<ImageUI> mapNameImage;

	shared_ptr<ImageUI> statBackground;
};

