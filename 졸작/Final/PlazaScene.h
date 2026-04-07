#pragma once
#include "Scene.h"

class PlazaScene final : public Scene
{
public:
	PlazaScene() = default;
	PlazaScene(const PlazaScene&) = delete;
	PlazaScene& operator=(const PlazaScene&) = delete;
	~PlazaScene() = default;

	void Release() override;
	void Reset() override;

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
	vector<shared_ptr<GameObject>> gameObjects;
	shared_ptr<MainCharacter> knight;
};

