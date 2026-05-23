#pragma once
#include "UIController.h"
#include "SceneManager.h"

class ImageUI;
class TextUI;
class UIComponent;

enum class PartyView { Lobby, Created };

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
	void InitJoinRequestPopup();

	void ShowPartyView(PartyView view);
	void RefreshMyPartyText();
	void RefreshPartyList();
	void UpdateJoinRequestPopup(float deltaTime);
	bool IsMyPartyLeader() const;

	static constexpr int MAX_PARTY_CARDS = 4;     // 로비에 띄울 파티 개수 상한
	static constexpr int MAX_PARTY_MEMBERS = 3;   // 한 파티의 멤버(정원) 상한

	SceneType sceneType;

	shared_ptr<ImageUI> statusBackImage;
	shared_ptr<ImageUI> statusImage;
	shared_ptr<ImageUI> statusRibbon;
	shared_ptr<ImageUI> statusArrowLeft;
	shared_ptr<ImageUI> statusArrowRight;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;
	shared_ptr<ImageUI> localCharPotion;

	shared_ptr<ImageUI> charHPBarBack;
	shared_ptr<ImageUI> charHPBar;

	shared_ptr<ImageUI> mapNameImage;  

	shared_ptr<ImageUI> mapBackImage;
	shared_ptr<ImageUI> mapImage;

	shared_ptr<ImageUI> partyBook;
	shared_ptr<ImageUI> partyListBox;
	shared_ptr<ImageUI> partyCreateButton;
	shared_ptr<ImageUI> partyJoinButton;
	shared_ptr<ImageUI> partyMyPartyBox;
	vector<shared_ptr<ImageUI>> partyMyCards;
	vector<shared_ptr<TextUI>>  partyMyLabels;
	vector<shared_ptr<ImageUI>> partyListCards;
	vector<shared_ptr<TextUI>>  partyListLabels;
	vector<shared_ptr<TextUI>>  partyListCountLabels;
	vector<uint64_t>            partyListCardIds;
	uint64_t selectedPartyId = 0;
	PartyView partyView = PartyView::Lobby;
	uint64_t lastPartyRevision = 0;

	shared_ptr<ImageUI> escWindow;
	shared_ptr<ImageUI> escContinueButton;
	shared_ptr<ImageUI> escOptionsButton;
	shared_ptr<ImageUI> escExitButton;

	shared_ptr<ImageUI> keyGuide;

	shared_ptr<ImageUI> settingWindow;
	shared_ptr<ImageUI> settingBackButton;

	shared_ptr<ImageUI> joinRequestWindow;
	shared_ptr<TextUI>  joinRequestText;
	shared_ptr<ImageUI> joinRequestOkButton;
	shared_ptr<ImageUI> joinRequestCancelButton;
	uint64_t            activeJoinRequestId = 0;

	// 신청서 팝업 슬라이드 인(오른쪽 화면 밖 → 제자리)
	vector<shared_ptr<UIComponent>> joinSlideWidgets;
	vector<float>                   joinSlideBaseX;
	float                           joinSlideDist = 0.0f;
	float                           joinSlideElapsed = 0.0f;
	static constexpr float          JOIN_SLIDE_DURATION = 0.25f;
};
