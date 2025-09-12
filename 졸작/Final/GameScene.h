#pragma once
#include "Scene.h"

class NetworkManager;

class GameScene final : public Scene
{
public:
	GameScene() = default;
	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;
	~GameScene();

	void SetNetworkManager(NetworkManager* nManager) { _nManager = nManager; }

	void Release() override;
	void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);
	void HandlePacket(const Protocol::GamePacket& packet);

protected:
	const float* GetBackgroundColor() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneEffects() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:
	NetworkManager* _nManager{ nullptr };

	vector<shared_ptr<GameObject>> gameObjects;

	shared_ptr<GameObject> dragon;
	shared_ptr<MainCharacter> knight;
	shared_ptr<GameObject> otherKnight;
	shared_ptr<GameObject> effectSample;
	shared_ptr<GameObject> effectSample2;
	shared_ptr<GameObject> effectSample3;
	shared_ptr<GameObject> effectSample4;
	shared_ptr<GameObject> flameEffect;
	shared_ptr<GameObject> fireWorkEffect;
	shared_ptr<GameObject> fireWorkEffect2;
	shared_ptr<GameObject> fireWorkEffect3;
};
