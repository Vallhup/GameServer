#pragma once
#include "Scene.h"

class SelectScene final : public Scene
{
public:
	SelectScene() = default;
	SelectScene(const SelectScene&) = delete;
	SelectScene& operator=(const SelectScene&) = delete;
	~SelectScene();

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
	shared_ptr<GameObject> dragon;
};
