#pragma once
#include "Scene.h"

class GameScene final : public Scene
{
public:
	GameScene() = default;
	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;
	~GameScene();

	void Release() override;
	void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);

protected:
	const float* GetBackgroundColor() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderScene() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:
	vector<shared_ptr<GameObject>> gameObjects;

	shared_ptr<GameObject> dragon;
	shared_ptr<MainCharacter> knight;
	shared_ptr<GameObject> knight2;
};
