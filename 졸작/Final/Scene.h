#pragma once
#include "Camera.h"
#include "SceneRenderer.h"
#include "InstancingBatch.h"
#include "GameObject.h"		// DX12Core.h 포함
#include "Mesh.h"			// Component.h 포함
#include "Transform.h"		// Component.h

class SceneManager;
enum class SceneType;
class MainCharacter;

class Scene
{
public:
	virtual ~Scene() {}
    virtual void Initialize(HWND hWnd, DX12Core& core);
    virtual void Update(const float deltaTime);
    virtual void RenderDeferred();
	virtual void RenderForward();
	virtual void RenderShadow();
	virtual void RenderEffects();
    virtual void Release() = 0;
    virtual void Reset() = 0;

	virtual const float* GetBackgroundColor() = 0;

	Camera* GetCamera() const;
	void SetSceneManager(SceneManager* manager);

protected:
	virtual void InitializeSceneObjectPools() = 0;
	virtual void InitializeLogic() = 0;
	virtual void UpdateScene(const float deltaTime) = 0;
	virtual void RenderSceneDeferred() = 0;
	virtual void RenderSceneForward() = 0;
	virtual void RenderSceneShadow() = 0;
	virtual void RenderSceneEffects() = 0;
	virtual void RequestSceneChange() = 0;

	template<typename T>
	shared_ptr<GameObject> CreateStaticMesh(const wstring& path, const T& data);

	template<typename T, size_t N>
	void CreateAndBatchObjects(const wstring& path, const T(&data)[N], vector<shared_ptr<InstancingBatch>>& targetBatchList);
	template<typename T>
	void CreateAndBatchObjects(const wstring& path, const vector<T>& data, vector<shared_ptr<InstancingBatch>>& targetBatchList);

protected:
	XMFLOAT4X4 mView = {};
	XMFLOAT4X4 mProjection = {};

	DX12Core* coreRef = nullptr;
	SceneManager* sManagerRef = nullptr;

	unique_ptr<Camera> cam;
};

template <typename T>
shared_ptr<GameObject> Scene::CreateStaticMesh(const wstring& path, const T& data)
{
	auto obj = make_shared<GameObject>();
	obj->SetId(0);
	obj->SetStatic(true);
	auto mesh = obj->AddComponent<Mesh>();
	auto transform = obj->AddComponent<Transform>();
	mesh->SetMesh(*coreRef, path);
	transform->SetInitPosition(data.position);
	transform->SetRotation(data.rotation);
	transform->SetScale(data.scale);
	return obj;
}

template <typename T, size_t N>
void Scene::CreateAndBatchObjects(const wstring& path, const T(&data)[N], vector<shared_ptr<InstancingBatch>>& targetBatchList)
{
	auto batch = make_shared<InstancingBatch>();

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

	for (int i = 0; i < data.size(); ++i)
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