#pragma once
#include "Scene.h"

class SkyBox;
class Terrain;

class PlazaScene final : public Scene
{
public:
	PlazaScene() = default;
	PlazaScene(const PlazaScene&) = delete;
	PlazaScene& operator=(const PlazaScene&) = delete;
	~PlazaScene() = default;

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
	void UpdateScene(const float deltaTime) override;

	const char* GetBGMPath() const override;

private:
	float SampleHeightAt(float worldX, float worldZ) const;

private:
	shared_ptr<SkyBox> skyBox;
	shared_ptr<Terrain> terrain;
};

