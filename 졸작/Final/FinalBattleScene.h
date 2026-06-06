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
	const char* GetBossBGMPath() const override;

private:
	float SampleHeightAt(float worldX, float worldZ) const;

private:
	shared_ptr<SkyBox> skyBox;
};

