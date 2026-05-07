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

	template<typename T>
	shared_ptr<GameObject> CreateStaticMesh(const wstring& path, const T& data);

	template<typename T, size_t N>
	void CreateAndBatchObjects(const wstring& path, const T(&data)[N], vector<shared_ptr<InstancingBatch>>& targetBatchList);
	template<typename T>
	void CreateAndBatchObjects(const wstring& path, const vector<T>& data, vector<shared_ptr<InstancingBatch>>& targetBatchList);

	void CreateCharacterPool(CharacterType type, int count = MAX_CHARACTER_COUNT);
	void CreateMonsters(MonsterType type, const XMFLOAT3& position, int count = 1);

private:
	shared_ptr<MainCharacter> GetAvailableCharacter(CharacterType type) const;
	shared_ptr<GameObject> GetAvailableMonster(MonsterType type) const;

	shared_ptr<MainCharacter> CreateCharacterObject(const wstring& meshPath, shared_ptr<AnimationSet>(*animFactory)());
	shared_ptr<GameObject> CreateMonsterObject(const wstring& meshPath, shared_ptr<AnimationSet>(*animFactory)(), bool twoSided = true);

	// Network Handler Function Interface
	void HandleLogin(const Protocol::SC_LOGIN_PACKET& login);
	void HandleAdd(const Protocol::SC_ADD_PACKET& add);
	void HandleMove(const Protocol::SC_MOVE_PACKET& move);
	void HandleRemove(const Protocol::SC_REMOVE_PACKET& remove);
	void HandleAnimationChange(const Protocol::SC_ANIMATION_TRANSITION_PACKET& anim);
	void HandleStatChange(const Protocol::SC_STAT_CHANGE_PACKET& stat);

protected:
	XMFLOAT4X4 mView = {};
	XMFLOAT4X4 mProjection = {};

	DX12Core* coreRef = nullptr;
	SceneManager* sManagerRef = nullptr;

	unique_ptr<Camera> cam;

	vector<shared_ptr<InstancingBatch>> instancingBatches;

	vector<shared_ptr<GameObject>> gameObjects;
	unordered_map<MonsterType, vector<shared_ptr<GameObject>>> monsterPools;
	unordered_map<CharacterType, vector<shared_ptr<MainCharacter>>> characterPools;

	unordered_map<int, shared_ptr<GameObject>> activeCharacters;
	shared_ptr<MainCharacter> myPlayer;

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

	if (path.find(L"SM_Mountain_A") != wstring::npos ||
		path.find(L"SM_Mountain_B") != wstring::npos ||
		path.find(L"SM_Mountain_C") != wstring::npos)
		batch->SetTerrainBlend(true);

	for (int i = 0; i < N; ++i)
	{
		auto obj = CreateStaticMesh(path, data[i]);

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

	if (path.find(L"SM_Mountain_A") != wstring::npos ||
		path.find(L"SM_Mountain_B") != wstring::npos ||
		path.find(L"SM_Mountain_C") != wstring::npos)
		batch->SetTerrainBlend(true);

	for (size_t i = 0; i < data.size(); ++i)
	{
		auto obj = CreateStaticMesh(path, data[i]);

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