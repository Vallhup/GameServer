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

	void CreateKnightPool();
	void CreateDragon();
	void CreateCastle();
	void CreateEffectSamples();
	shared_ptr<MainCharacter> GetAvailableKnight() const;

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

	vector<shared_ptr<GameObject>> gameObjects;

	shared_ptr<GameObject> dragon;

	vector<shared_ptr<MainCharacter>> knightPool;
	static constexpr int MAX_KNIGHT_COUNT = 100;
	unordered_map<int, shared_ptr<MainCharacter>> activePlayers;
	shared_ptr<MainCharacter> myPlayer;

	vector<shared_ptr<GameObject>> effectObjects;
};
