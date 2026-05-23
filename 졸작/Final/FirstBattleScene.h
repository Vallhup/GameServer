#pragma once
#include "Scene.h"

class Terrain;
class SkyBox;
class Water;

class FirstBattleScene final : public Scene
{
public:
	FirstBattleScene() = default;
	FirstBattleScene(const FirstBattleScene&) = delete;
	FirstBattleScene& operator=(const FirstBattleScene&) = delete;
	~FirstBattleScene() = default;

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
	shared_ptr<SkyBox> skyBox;
	shared_ptr<Terrain> terrain;
	shared_ptr<Terrain> oceanFloor;
	shared_ptr<Water> water;
};
