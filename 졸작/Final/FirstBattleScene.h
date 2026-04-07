#pragma once
#include "Scene.h"

class NetworkManager;
class VertexIndexBuffer;
class SkyBox;
class Terrain;
class Water;
class TrailRenderer;
class FootDustEffect;
class FlameEffect;
class ParrySparkEffect;

class FirstBattleScene final : public Scene
{
public:
	FirstBattleScene() = default;
	FirstBattleScene(const FirstBattleScene&) = delete;
	FirstBattleScene& operator=(const FirstBattleScene&) = delete;
	~FirstBattleScene() = default;

	shared_ptr<MainCharacter> GetAvailableKnight() const;
	shared_ptr<MainCharacter> GetMyPlayer() const;

	void SetNetworkManager(NetworkManager* nManager) { _nManager = nManager; }

	void Release() override;
	void Reset() override;

	void AddGameObject(shared_ptr<GameObject> obj);

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
	void CreateKnightPool();
	void CreateBossObject();
	void CreateImpObject();
	void CreateMap();
	void CreateEffectSamples();
	
	float SampleHeightAt(float worldX, float worldZ) const;

	// Network Handler Function Override
	void HandleLogin(const Protocol::SC_LOGIN_PACKET& login) override;
	void HandleAdd(const Protocol::SC_ADD_PACKET& add) override;
	void HandleMove(const Protocol::SC_MOVE_PACKET& move) override;
	void HandleRemove(const Protocol::SC_REMOVE_PACKET& remove) override;
	void HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim) override;
	void HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat) override;

private:
	NetworkManager* _nManager{ nullptr };

	vector<shared_ptr<GameObject>> gameObjects;

	vector<shared_ptr<MainCharacter>> knightPool;
	static constexpr int MAX_KNIGHT_COUNT = 151;
	unordered_map<int, shared_ptr<GameObject>> activeCharacters;
	shared_ptr<MainCharacter> myPlayer;

	shared_ptr<GameObject> bossObject;
	shared_ptr<GameObject> impObject;

	vector<shared_ptr<GameObject>> effectObjects;

	shared_ptr<SkyBox> skyBox;

	shared_ptr<Terrain> terrain;
	shared_ptr<Terrain> oceanFloor;
	shared_ptr<Water> water;

	unique_ptr<TrailRenderer> trailRenderer;
	unique_ptr<FootDustEffect> footDustEffect;
	unique_ptr<FlameEffect> flameEffect;
	unique_ptr<ParrySparkEffect> parryEffect;

	bool rightFootSpawned = false;
	bool leftFootSpawned = false;
	bool parrySparkSpawned = false;
};
