#pragma once
#include "Scene.h"

class NetworkManager;
class VertexIndexBuffer;
class SkyBox;

class GameScene final : public Scene
{
public:
	GameScene() = default;
	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;
	~GameScene();

	shared_ptr<MainCharacter> GetAvailableKnight() const;

	void SetNetworkManager(NetworkManager* nManager) { _nManager = nManager; }

	void Release() override;
	void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);
	void HandlePacket(const PacketHeader& header, const BYTE* data);

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
	void CreateKnightPool();
	shared_ptr<GameObject> CreateStaticMesh(const wstring& path, const XMFLOAT3& pos = { 0.0f, 0.0f, 0.0f }, const XMFLOAT3& rot = { 0.0f, 0.0f, 0.0f }, const XMFLOAT3& scale = { 0.01f, 0.01f, 0.01f });
	void CreateCastle();
	void CreateEffectSamples();

private:
	NetworkManager* _nManager{ nullptr };

	vector<shared_ptr<GameObject>> gameObjects;

	vector<shared_ptr<MainCharacter>> knightPool;
	static constexpr int MAX_KNIGHT_COUNT = 10;
	unordered_map<int, shared_ptr<MainCharacter>> activePlayers;
	shared_ptr<MainCharacter> myPlayer;

	vector<shared_ptr<GameObject>> effectObjects;

	shared_ptr<SkyBox> skyBox;

	vector<InstanceGroup> instanceGroups;
};
