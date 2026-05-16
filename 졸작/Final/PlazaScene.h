#pragma once
#include "Scene.h"

class NetworkManager;
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

	void SetNetworkManager(NetworkManager* nManager) { _nManager = nManager; }

protected:
	void InitializeLogic() override;
	void InitializeSceneEnvironments() override;
	void UpdateScene(const float deltaTime) override;
	void RequestSceneChange() override;

private:
	void CreateEffectSamples();

	float SampleHeightAt(float worldX, float worldZ) const;

private:
	NetworkManager* _nManager{ nullptr };

	vector<shared_ptr<GameObject>> effectObjects;
	shared_ptr<SkyBox> skyBox;
	shared_ptr<Terrain> terrain;
};

