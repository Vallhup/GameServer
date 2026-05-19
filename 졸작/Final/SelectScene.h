#pragma once
#include "Scene.h"

class SelectScene final : public Scene
{
public:
	SelectScene() = default;
	SelectScene(const SelectScene&) = delete;
	SelectScene& operator=(const SelectScene&) = delete;
	~SelectScene() = default;

	void RenderSceneDeferred() override;

	void Release() override;

protected:
	void InitializeLogic() override;
	void InitializeSceneMonsters() override;
	void UpdateScene(const float deltaTime) override;

private:
	vector<shared_ptr<GameObject>> gameObjects;
	shared_ptr<MainCharacter> bigDemonWarrior;
	shared_ptr<GameObject> tank;
};
