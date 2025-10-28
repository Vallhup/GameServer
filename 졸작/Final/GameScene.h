#pragma once
#include "Scene.h"
#include "ObjectPoolManager.h"

class NetworkManager;

class GameScene final : public Scene
{
public:
	GameScene() = default;
	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;
	~GameScene();

	void CreateDragon();
	void CreateCastle();
	void CreateEffectSamples();

	void SetNetworkManager(NetworkManager* nManager) { _nManager = nManager; }

	void Release() override;
	void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);
	void HandlePacket(const Protocol::GamePacket& packet);

protected:
	const float* GetBackgroundColor() override;
	void InitializeSceneObjectPools() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:
	NetworkManager* _nManager{ nullptr };
	ObjectPoolManager objManager;

	vector<shared_ptr<GameObject>> gameObjects;
	shared_ptr<GameObject> dragon;
	shared_ptr<MainCharacter> myPlayer;
	vector<shared_ptr<GameObject>> effectObjects;
};
