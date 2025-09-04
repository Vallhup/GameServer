#pragma once

#include <unordered_map>
#include <queue>
#include "Scene.h"
#include "NetworkManager.h"

class ServerTestScene final : public Scene {
public:
	ServerTestScene() = default;
	~ServerTestScene() = default;

	void SetNetworkManager(NetworkManager* nManager) { _nManager = nManager; }

public:
	virtual void Release() override;
	virtual void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);
	void HandlePacket(const Protocol::GamePacket& packet);

public:
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
	int _myId = 0;
	vector<shared_ptr<GameObject>> gameObjects;  
	shared_ptr<MainCharacter> knight;
	shared_ptr<GameObject> otherKnight;
};

