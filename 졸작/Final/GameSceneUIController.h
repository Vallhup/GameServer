#pragma once
#include "UIController.h"
#include "SceneManager.h"

class ImageUI;
class TextUI;
class UIComponent;
class GameObject;
enum class MonsterType;

enum class PartyView { Lobby, Created };

struct EndingBeat
{
	wstring bg;    
	wstring story; 
};

enum class EndingPhase
{
	None,
	IntroBlackIn, IntroReveal,
	BeatDelay, StoryIn, StoryHold, StoryOut,
	SwapDelay,
	EndDelay, EndFade,
	CreditsRoll, CreditsHold,
};

struct MonsterBarTarget { 
	GameObject* obj = nullptr; 
	float hpPercent = 1.0f; 
	bool inCombat = false; 
};

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
	void SetMonsterCombatState(int id, bool inCombat);
	void RemoveMonsterBar(int id);

	void InitBossHpBar();
	void HandleBossHp(int cur, int max);
	void SetBossCombatState(bool inCombat, MonsterType bossType);
	void RemoveBossHpBar();

	void HandlePartyMemberHp(int id, int cur, int max);

	void SetPotionCount(uint32_t count);
	void SetDeathCount(uint32_t death, uint32_t max);

	void ShowMapName();

	void ShowPvpOverlay();

	void SetInteractPrompt(bool active, const XMFLOAT3& worldAnchor);
	void SetBoardPrompt(bool active, const XMFLOAT3& anchor, const XMFLOAT3& focusEye, const XMFLOAT3& focusLook);

	bool ConsumeBeaconConfirmed();

	void HideHudForCinematic();

	void PlayHappyEnding();
	void PlayPvpEnding();

	void ShowHeroChoice(uint64_t voteId);
	void HideHeroChoice();					

private:
	void InitMonsterHpBars();
	void UpdateMonsterHpBars();
	void InitLocalPlayerHUD();
	void InitMapNameOverlay();
	void InitPvpOverlay();
	void UpdatePvpOverlay(float deltaTime);
	void InitEnding();
	void InitCredits();
	void UpdateEnding(float deltaTime);
	void StartEnding(vector<EndingBeat> beats, const char* bgmPath,
		Protocol::FinalEndingCinematicContext doneContext);
	void CenterStoryImage(const wstring& texName);
	void InitPartyWindow();
	void InitStatWindow();
	void RefreshTitleRibbon();
	void InitMapWindow();
	void InitEscWindow();
	void InitKeyGuide();
	void InitSettingWindow();
	void InitJoinRequestPopup();
	void InitInteractPrompt();
	void UpdateInteractPrompt();
	void InitStatueWindow();
	void UpdateStatueWindow();
	void UpdateBoardFocus();
	void InitBeaconWindow();
	void UpdateBeaconWindow();
	void InitHeroChoiceWindow();
	void UpdateHeroChoiceWindow();
	void InitRespawnWindow();
	void UpdateRespawnWindow(float deltaTime);
	void OnLocalPlayerDied();
	void OnLocalPlayerRevived();
	void RequestRespawn();

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
	shared_ptr<TextUI>  statusStatText;
	shared_ptr<ImageUI> statusTitleImage;
	uint64_t            lastTitleRevision = 0;
	wstring             lastStatText;
	shared_ptr<ImageUI> localCharBarsBack;
	shared_ptr<ImageUI> localCharHpBar;
	shared_ptr<ImageUI> localCharStaminaBar;
	shared_ptr<ImageUI> localCharPotion;
	shared_ptr<TextUI>  localCharPotionCount;
	shared_ptr<ImageUI> localCharDeathCount;
	shared_ptr<TextUI>  localCharDeathCountText;

	unordered_map<int, MonsterBarTarget> monsterHpTargets;
	vector<shared_ptr<ImageUI>> monsterBarBacks;
	vector<shared_ptr<ImageUI>> monsterBars;
	static constexpr int MAX_MONSTER_HP_BARS = 16;

	shared_ptr<ImageUI> bossBarBack;
	shared_ptr<ImageUI> bossBar;
	shared_ptr<TextUI>  bossNameLabel;
	float bossBarFullW = 0.0f;
	float bossHpPercent = 1.0f;
	bool bossInCombat = false;

	shared_ptr<ImageUI> mapNameImage;

	shared_ptr<ImageUI> pvpOverlayImage;
	bool                pvpOverlayPending = false;
	float               pvpOverlayTimer = 0.0f;
	static constexpr float PVP_OVERLAY_DELAY = 3.0f;

	shared_ptr<ImageUI> endingBg;
	shared_ptr<ImageUI> endingStory;
	shared_ptr<ImageUI> endingBlack;
	EndingPhase         endingPhase = EndingPhase::None;
	float               endingTimer = 0.0f;
	vector<EndingBeat>  endingBeats;
	size_t              endingBeatIndex = 0;
	Protocol::FinalEndingCinematicContext endingDoneContext =
		Protocol::FINAL_ENDING_CINEMATIC_CONTEXT_FINAL_CLEAR;
	static constexpr float ENDING_BG_DELAY   = 2.0f;  
	static constexpr float ENDING_STORY_HOLD = 6.0f;  
	static constexpr float ENDING_STORY_GAP  = 1.0f;  
	static constexpr float ENDING_SWAP_DELAY = 1.0f;  
	static constexpr float ENDING_END_DELAY  = 2.0f;  
	static constexpr float ENDING_FADE       = 2.0f;  

	vector<shared_ptr<TextUI>> creditLines;
	float creditScrollOffset = 0.0f;  
	float creditScrollSpeed  = 0.0f;  
	float creditScrollEnd    = 0.0f;  
	float creditLineHeight   = 0.0f;
	static constexpr float CREDIT_SCROLL_SECONDS = 80.0f;  
	static constexpr float CREDIT_END_HOLD       = 8.0f;   
	static constexpr float CREDIT_FONT_SCALE     = 1.2f;   

	shared_ptr<ImageUI> mapBackImage;
	shared_ptr<ImageUI> mapImage;
	shared_ptr<TextUI>  mapLegendText;

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

	bool                boardActive = false;
	bool                boardFocused = false;
	XMFLOAT3            boardAnchor{};
	XMFLOAT3            boardFocusEye{};
	XMFLOAT3            boardFocusLook{};

	shared_ptr<ImageUI> statueWindow;
	shared_ptr<ImageUI> statueOkButton;
	shared_ptr<ImageUI> statueCancelButton;

	shared_ptr<ImageUI> beaconWindow;
	shared_ptr<ImageUI> beaconOkButton;
	shared_ptr<ImageUI> beaconCancelButton;
	bool                beaconConfirmed = false;

	shared_ptr<ImageUI> respawnWindow;
	shared_ptr<TextUI>  respawnCountText;
	shared_ptr<ImageUI> respawnOkButton;
	bool                respawnActive = false;
	bool                localDeadHandled = false;
	float               respawnTimer = 0.0f;
	uint32_t            deathCount = 0;
	static constexpr float RESPAWN_SECONDS = 5.0f;  

	shared_ptr<ImageUI> heroChoiceWindow;
	shared_ptr<ImageUI> heroMeButton;
	shared_ptr<ImageUI> heroWeButton;
	uint64_t            heroChoiceVoteId = 0;

	shared_ptr<ImageUI> settingWindow;

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
