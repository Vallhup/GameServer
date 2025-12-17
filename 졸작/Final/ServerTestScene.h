#pragma once
#include "Scene.h"

class ServerTestScene final : public Scene {
public:
	ServerTestScene() = default;
	~ServerTestScene() = default;

public:
	virtual void Release() override;
	virtual void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);

public:
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

