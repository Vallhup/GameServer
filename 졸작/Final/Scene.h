#pragma once
#include "Camera.h"
#include "SceneRenderer.h"
#include "InstancingBatch.h"
#include "GameObject.h"		// DX12Core.h 포함
#include "Mesh.h"			// Component.h 포함
#include "Transform.h"		// Component.h
#include "SceneSetting.h"

class SceneManager;
enum class SceneType;
class MainCharacter;
class AnimationSet;
class GimmickDiamond;

enum class CharacterType { Knight, Lancer, Paladin };
enum class MonsterType { Boss, Imp, DemonStriker, DemonExecutioner, BigDemonWarrior, Tank };

class Scene
{
public:
	virtual ~Scene() {}
	virtual void Initialize(HWND hWnd, DX12Core& core);
	virtual void Update(const float deltaTime);
	virtual void RenderSceneDeferred() {}
	virtual void RenderSceneForward() {}
	virtual void RenderSceneShadowStatic() {}
	virtual void RenderSceneShadowDynamic() {}
	virtual void RenderSceneEffects() {}
	virtual void Release() = 0;

	Camera* GetCamera() const;
	MainCharacter* GetMyPlayer() const { return myPlayer.get(); }
	CharacterType GetMyCharacterType() const { return myCharacterType; }
	void SetSceneManager(SceneManager* manager);
	void HandlePacket(const PacketHeader& header, const BYTE* data);
	void SetInstancingBatches(vector<shared_ptr<InstancingBatch>>&& batches);

	void AddGameObject(shared_ptr<GameObject> obj);

	virtual SceneSettings GetSceneSettings() const { return {}; }

protected:
	virtual void InitializeLogic() = 0;
	virtual void InitializeSceneEnvironments() {}
	virtual void InitializeSceneMonsters() {}
	virtual void UpdateScene(const float deltaTime) {}
	virtual void RequestSceneChange() {}

	virtual const char* GetBGMPath() const { return nullptr; }
	virtual const char* GetBossBGMPath() const { return nullptr; }
	virtual float GetBGMFadeInSeconds() const { return 0.0f; }

	template<typename T>
	shared_ptr<GameObject> CreateStaticMesh(const wstring& path, const T& data);

	template<typename T, size_t N>
	void CreateAndBatchObjects(const wstring& path, const T(&data)[N], vector<shared_ptr<InstancingBatch>>& targetBatchList);
	template<typename T>
	void CreateAndBatchObjects(const wstring& path, const vector<T>& data, vector<shared_ptr<InstancingBatch>>& targetBatchList);

	void CreateCharacterPool(CharacterType type, int count = MAX_CHARACTER_COUNT);
	void CreateMonsters(MonsterType type, const XMFLOAT3& position, int count = 1);
	void CreateGimmickPool(int count);	

private:
	shared_ptr<MainCharacter> GetAvailableCharacter(CharacterType type) const;
	shared_ptr<GameObject> GetAvailableMonster(MonsterType type) const;
	shared_ptr<GimmickDiamond> GetAvailableGimmick();

	shared_ptr<MainCharacter> CreateCharacterObject(const wstring& meshPath, shared_ptr<AnimationSet>(*animFactory)());
	shared_ptr<GameObject> CreateMonsterObject(const wstring& meshPath, shared_ptr<AnimationSet>(*animFactory)(), bool twoSided = true);

	// Network Handler Function Interface
	void HandleLoginSuccess(const Protocol::SC_LOGIN_SUCCESS_PACKET& success);
	void HandleLoginFail(const Protocol::SC_LOGIN_FAIL_PACKET& fail);
	void HandleAdd(const Protocol::SC_ADD_PACKET& add);
	void HandleMove(const Protocol::SC_MOVE_PACKET& move);
	void HandleRemove(const Protocol::SC_REMOVE_PACKET& remove);
	void HandleCombatImpact(const Protocol::SC_COMBAT_IMPACT_PACKET& impact);
	void HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim);
	void HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat);
	void HandleItemCount(const Protocol::SC_ITEM_COUNT_PACKET& itemCount);
	void HandleTeamDeathCount(const Protocol::SC_TEAM_DEATH_COUNT_PACKET& deathCount);
	void HandleMonsterCombatState(const Protocol::SC_MONSTER_COMBAT_STATE_PACKET& combatState);
	void HandleBossGimmickObjectSync(const Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET& gimmickObject);
	void HandleBossGimmickZoneSync(const Protocol::SC_BOSS_GIMMICK_ZONE_SYNC_PACKET& gimmickZone);
	void HandleFinalClearChoiceBegin(const Protocol::SC_FINAL_CLEAR_CHOICE_BEGIN_PACKET& choiceBegin);
	void HandleFinalClearChoiceResult(const Protocol::SC_FINAL_CLEAR_CHOICE_RESULT_PACKET& choiceResult);

	void UpdateDissolves();

protected:
	DX12Core* coreRef = nullptr;
	SceneManager* sManagerRef = nullptr;

	unique_ptr<Camera> cam;

	vector<shared_ptr<InstancingBatch>> instancingBatches;

	vector<shared_ptr<GameObject>> gameObjects;
	unordered_map<MonsterType, vector<shared_ptr<GameObject>>> monsterPools;
	unordered_map<CharacterType, vector<shared_ptr<MainCharacter>>> characterPools;

	unordered_map<int, shared_ptr<GameObject>> activeCharacters;
	unordered_map<int, MonsterType> activeMonsterTypes;

	vector<shared_ptr<GimmickDiamond>> gimmickPool;
	unordered_map<int, shared_ptr<GimmickDiamond>> activeGimmicks;
	unordered_map<int, int> activeZoneBarriers;	
	shared_ptr<MainCharacter> myPlayer;
	CharacterType myCharacterType = CharacterType::Knight;

	bool bgmStarted = false;

	static constexpr int MAX_CHARACTER_COUNT = 5;
};

template <typename T>
shared_ptr<GameObject> Scene::CreateStaticMesh(const wstring& path, const T& data)
{
	auto obj = make_shared<GameObject>();
	obj->SetId(0);
	obj->SetStatic(true);
	obj->SetDistanceCull(data.distanceCull, data.cullDistance);
	auto mesh = obj->AddComponent<Mesh>();
	auto transform = obj->AddComponent<Transform>();
	mesh->SetMesh2(*coreRef, path);
	transform->SetInitPosition(data.position);
	transform->SetRotation(data.rotation);
	transform->SetScale(data.scale);
	return obj;
}

template <typename T, size_t N>
void Scene::CreateAndBatchObjects(const wstring& path, const T(&data)[N], vector<shared_ptr<InstancingBatch>>& targetBatchList)
{
	auto batch = make_shared<InstancingBatch>();
	batch->SetCastShadow(data[0].castShadow);
	batch->SetTwoSided(data[0].twoSided);
	batch->SetVertexAnim(data[0].vertexAnim);

	const bool isMountain = path.find(L"SM_Mountain_A") != wstring::npos ||
		path.find(L"SM_Mountain_B") != wstring::npos ||
		path.find(L"SM_Mountain_C") != wstring::npos;
	if (isMountain)
		batch->SetTerrainBlend(true);

	for (int i = 0; i < N; ++i)
	{
		auto obj = CreateStaticMesh(path, data[i]);

		if (isMountain)
			obj->SetObstructsCamera(false);

		if (i == 0)
		{
			if (auto mesh = obj->GetComponent<Mesh>())
				batch->Initialize(mesh);
		}

		batch->AddObject(obj);
	}

	batch->BuildBuffers(*coreRef);
	targetBatchList.push_back(move(batch));
}

template <typename T>
void Scene::CreateAndBatchObjects(const wstring& path, const vector<T>& data, vector<shared_ptr<InstancingBatch>>& targetBatchList)
{
	if (data.empty()) return;

	auto batch = make_shared<InstancingBatch>();
	batch->SetCastShadow(data[0].castShadow);
	batch->SetTwoSided(data[0].twoSided);
	batch->SetVertexAnim(data[0].vertexAnim);

	const bool isMountain = path.find(L"SM_Mountain_A") != wstring::npos ||
		path.find(L"SM_Mountain_B") != wstring::npos ||
		path.find(L"SM_Mountain_C") != wstring::npos;
	if (isMountain)
		batch->SetTerrainBlend(true);

	for (size_t i = 0; i < data.size(); ++i)
	{
		auto obj = CreateStaticMesh(path, data[i]);

		if (isMountain)
			obj->SetObstructsCamera(false);

		if (i == 0)
		{
			if (auto mesh = obj->GetComponent<Mesh>())
				batch->Initialize(mesh);
		}

		batch->AddObject(obj);
	}

	batch->BuildBuffers(*coreRef);
	targetBatchList.push_back(move(batch));
}