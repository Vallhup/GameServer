#pragma once
#include "Scene.h"

class TownScene final : public Scene
{
public:
	TownScene() = default;
	TownScene(const TownScene&) = delete;
	TownScene& operator=(const TownScene&) = delete;
	~TownScene();

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
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

