#pragma once
#include "Scene.h"

class SkyBox;

class FinalBattleScene final : public Scene
{
public:
	FinalBattleScene() = default;
	FinalBattleScene(const FinalBattleScene&) = delete;
	FinalBattleScene& operator=(const FinalBattleScene&) = delete;
	~FinalBattleScene() = default;

	shared_ptr<MainCharacter> GetAvailableKnight() const;

	void Release() override;
	void Reset() override;

	SceneSettings GetSceneSettings() const override;

protected:
	void InitializeSceneObjectPools() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;

private:
	void CreateKnightPool();
	float SampleHeightAt(float worldX, float worldZ) const;

	// Network Handler Function Override
	void HandleLogin(const Protocol::SC_LOGIN_PACKET& login) override;
	void HandleAdd(const Protocol::SC_ADD_PACKET& add) override;
	void HandleMove(const Protocol::SC_MOVE_PACKET& move) override;
	void HandleRemove(const Protocol::SC_REMOVE_PACKET& remove) override;
	void HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim) override;
	void HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat) override;

private:
	vector<shared_ptr<MainCharacter>> knightPool;
	static constexpr int MAX_KNIGHT_COUNT = 10;

	unordered_map<int, shared_ptr<GameObject>> activeCharacters;

	shared_ptr<MainCharacter> myPlayer;

	shared_ptr<SkyBox> skyBox;
};

