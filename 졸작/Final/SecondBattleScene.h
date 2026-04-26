#pragma once
#include "Scene.h"

class Terrain;
class SkyBox;
class Water;

class SecondBattleScene final : public Scene
{
public:
	SecondBattleScene() = default;
	SecondBattleScene(const SecondBattleScene&) = delete;
	SecondBattleScene& operator=(const SecondBattleScene&) = delete;
	~SecondBattleScene() = default;

	void Release() override;
	void Reset() override;

	SceneSettings GetSceneSettings() const override;

protected:
	void InitializeLogic() override;
	void InitializeSceneEnvironments() override;
	void InitializeSceneMonsters() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;

private:
	float SampleHeightAt(float worldX, float worldZ) const;

	// Network Handler Function Override
	void HandleLogin(const Protocol::SC_LOGIN_PACKET& login) override;
	void HandleAdd(const Protocol::SC_ADD_PACKET& add) override;
	void HandleMove(const Protocol::SC_MOVE_PACKET& move) override;
	void HandleRemove(const Protocol::SC_REMOVE_PACKET& remove) override;
	void HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim) override;
	void HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat) override;

private:
	shared_ptr<Terrain> terrain;
	shared_ptr<SkyBox> skyBox;
	shared_ptr<Water> water;
};

