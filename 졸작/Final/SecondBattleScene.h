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

	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadowStatic() override;
	void RenderSceneShadowDynamic() override;
	void RenderSceneEffects() override;

	void Release() override;

	SceneSettings GetSceneSettings() const override;

protected:
	void InitializeLogic() override;
	void InitializeSceneEnvironments() override;
	void InitializeSceneMonsters() override;
	void UpdateScene(const float deltaTime) override;
	void RequestSceneChange() override;

	const char* GetBGMPath() const override;

private:
	float SampleHeightAt(float worldX, float worldZ) const;

private:
	shared_ptr<Terrain> terrain;
	shared_ptr<SkyBox> skyBox;
	shared_ptr<Water> water;
};

