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

	void Release() override;

	SceneSettings GetSceneSettings() const override;

protected:
	void InitializeLogic() override;
	void InitializeSceneEnvironments() override;
	void InitializeSceneMonsters() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadowStatic() override;
	void RenderSceneShadowDynamic() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;

private:
	void CreateBossCharacter();

	float SampleHeightAt(float worldX, float worldZ) const;

	// Network Handler Function Override
	void HandleLogin(const Protocol::SC_LOGIN_PACKET& login) override;
	void HandleAdd(const Protocol::SC_ADD_PACKET& add) override;
	void HandleMove(const Protocol::SC_MOVE_PACKET& move) override;
	void HandleRemove(const Protocol::SC_REMOVE_PACKET& remove) override;
	void HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim) override;
	void HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat) override;

private:
	shared_ptr<SkyBox> skyBox;
};

