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

	void HandleStatBarChange(int curHp, int maxHp, int curStamina, int maxStamina);
	void HandleStatImageChange(
		int curHp, int maxHp, int curStamina, int maxStamina,
		int power, double aSpeed, int defense, double mSpeed);
	bool IsStatWindowOn() const;

	void ShowMapName();

private:
	void InitTargetHpBar();
	void InitLocalPlayerHUD();
	void InitMapNameOverlay();
	void InitPartyWindow();
	void InitStatWindow();
	void InitMapWindow();
	void InitEscWindow();
	void InitKeyGuide();
	void InitSettingWindow();

	SceneType sceneType;

	shared_ptr<ImageUI> statusBackImage;
	shared_ptr<ImageUI> statusImage;
	shared_ptr<ImageUI> statusRibbon;
	shared_ptr<ImageUI> statusArrowLeft;
	shared_ptr<ImageUI> statusArrowRight;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;

	shared_ptr<TextUI>  tempStatusText;

	shared_ptr<ImageUI> charHPBarBack;
	shared_ptr<ImageUI> charHPBar;

	shared_ptr<ImageUI> mapNameImage;  

	shared_ptr<ImageUI> mapBackImage;
	shared_ptr<ImageUI> mapImage;

	shared_ptr<ImageUI> partyBook;
	shared_ptr<ImageUI> partyListBox;
	shared_ptr<ImageUI> partyCreateButton;
	shared_ptr<ImageUI> partyJoinButton;

	shared_ptr<ImageUI> escWindow;
	shared_ptr<ImageUI> escContinueButton;
	shared_ptr<ImageUI> escOptionsButton;
	shared_ptr<ImageUI> escExitButton;

	shared_ptr<ImageUI> keyGuide;

	shared_ptr<ImageUI> settingWindow;
	shared_ptr<ImageUI> settingBackButton;
};
