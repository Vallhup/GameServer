#pragma once
#include "UIController.h"
#include "SceneManager.h"

class ImageUI;
class TextUI;
class UIComponent;
class GameObject;

enum class PartyView { Lobby, Created };

struct MonsterBarTarget { GameObject* obj = nullptr; float hpPercent = 1.0f; };

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

	void SetLocalCharacterType(CharacterType type);
	void HandleMonsterHp(int id, GameObject* obj, int cur, int max);
	void HandlePartyMemberHp(int id, int cur, int max);

	void ShowMapName();

	void SetInteractPrompt(bool active, const XMFLOAT3& worldAnchor);

	bool ConsumeBeaconConfirmed();

private:
	void InitMonsterHpBars();
	void UpdateMonsterHpBars();
	void InitLocalPlayerHUD();
	void InitMapNameOverlay();
	void InitPartyWindow();
	void InitStatWindow();
	void InitMapWindow();
	void InitEscWindow();
	void InitKeyGuide();
	void InitSettingWindow();
	void InitJoinRequestPopup();
	void InitInteractPrompt();
	void UpdateInteractPrompt();
	void InitStatueWindow();
	void UpdateStatueWindow();
	void InitBeaconWindow();
	void UpdateBeaconWindow();

	void ShowPartyView(PartyView view);
	void RefreshMyPartyText();
	void RefreshPartyList();
	void UpdateJoinRequestPopup(float deltaTime);
	bool IsMyPartyLeader() const;

	void InitPartyMemberHud();
	void RefreshPartyMemberHud();

	static constexpr int MAX_PARTY_CARDS = 4;     
	static constexpr int MAX_PARTY_MEMBERS = 3;   
	static constexpr int PARTY_HUD_SLOTS = MAX_PARTY_MEMBERS; 

	SceneType sceneType;

	shared_ptr<ImageUI> statusBackImage;
	shared_ptr<ImageUI> statusCharImage;   
	shared_ptr<ImageUI> statusImage;
	shared_ptr<ImageUI> statusRibbon;
	shared_ptr<ImageUI> statusArrowLeft;
	shared_ptr<ImageUI> statusArrowRight;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;
	shared_ptr<ImageUI> localCharPotion;

	unordered_map<int, MonsterBarTarget> monsterHpTargets;
	vector<shared_ptr<ImageUI>> monsterBarBacks;
	vector<shared_ptr<ImageUI>> monsterBars;
	static constexpr int MAX_MONSTER_HP_BARS = 16;

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

	shared_ptr<ImageUI>         partyHudBack;
	vector<shared_ptr<ImageUI>> partyHudIcons;
	vector<shared_ptr<TextUI>>  partyHudNames;
	vector<shared_ptr<ImageUI>> partyHudBarBacks;
	vector<shared_ptr<ImageUI>> partyHudBars;
	vector<float>               partyHudBarFullW;
	vector<int>                 partyHudSlotIds;       
	unordered_map<int, float>   partyMemberHpPercent;  
	bool     partyHudUserVisible = true;
	bool     partyHudInParty = false;
	bool     partyHudDirty = false;
	uint64_t partyHudRevision = 0;
	float    partyHudSelfHpPercent = 1.0f;

	shared_ptr<ImageUI> escWindow;
	shared_ptr<ImageUI> escContinueButton;
	shared_ptr<ImageUI> escOptionsButton;
	shared_ptr<ImageUI> escExitButton;

	shared_ptr<ImageUI> keyGuide;

	shared_ptr<ImageUI> interactCircle;
	shared_ptr<TextUI>  interactKeyText;
	shared_ptr<TextUI>  interactLabelText;
	bool                interactActive = false;
	float               interactScale = 1.0f;
	XMFLOAT3            interactWorldAnchor{};

	shared_ptr<ImageUI> statueWindow;
	shared_ptr<ImageUI> statueOkButton;
	shared_ptr<ImageUI> statueCancelButton;

	shared_ptr<ImageUI> beaconWindow;
	shared_ptr<ImageUI> beaconOkButton;
	shared_ptr<ImageUI> beaconCancelButton;
	bool                beaconConfirmed = false;

	shared_ptr<ImageUI> settingWindow;
	shared_ptr<ImageUI> settingBackButton;

	shared_ptr<ImageUI> joinRequestWindow;
	shared_ptr<TextUI>  joinRequestText;
	shared_ptr<ImageUI> joinRequestOkButton;
	shared_ptr<ImageUI> joinRequestCancelButton;
	uint64_t            activeJoinRequestId = 0;

	vector<shared_ptr<UIComponent>> joinSlideWidgets;
	vector<float>                   joinSlideBaseX;
	float                           joinSlideDist = 0.0f;
	float                           joinSlideElapsed = 0.0f;
	static constexpr float          JOIN_SLIDE_DURATION = 0.25f;
};
