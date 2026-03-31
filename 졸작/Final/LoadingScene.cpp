#include "pch.h"
#include "LoadingScene.h"
#include "Material.h"
#include "InstanceLoader.h"
#include "Engine.h"
#include "UIManager.h"
#include "LoadingSceneUIController.h"
#include "DX12Core.h"
#include "SceneManager.h"

LoadingScene::~LoadingScene() = default;

void LoadingScene::Release()
{
}

void LoadingScene::Reset()
{
	Material::ReleaseUploadBuffers();
	while (!loadTasks.empty()) loadTasks.pop();
	totalTasks = 0;
	completedTasks = 0;
	OutputDebugStringA("LoadingScene Data has been deleted!! \n----------------------------------------\n");
}

vector<shared_ptr<InstancingBatch>> LoadingScene::TakeBatches(SceneType type)
{
	auto it = sceneBatches.find(type);
	if (it != sceneBatches.end())
		return move(it->second);
	return {};
}

void LoadingScene::InitializeSceneObjectPools()
{
}

void LoadingScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nLoadingScene Data has been created!! \n");

	auto controller = ENGINE.GetUIManager()->GetController<LoadingSceneUIController>(SceneType::Loading);
	controller->SetTargetScene(targetScene);

	switch (targetScene)
	{
	case SceneType::Select:
		LoadSelectGameResources();
		break;
	case SceneType::Town:
		LoadTownGameResources();
		break;
	case SceneType::MainGame:
		LoadMainGameResources();
		break;
	}
}

void LoadingScene::UpdateScene(const float deltaTime)
{
	if (!loadTasks.empty()) {
		loadTasks.front()();
		loadTasks.pop();

		coreRef->ExecuteLoadingCommands();

		auto& batches = sceneBatches[targetScene];
		if (!batches.empty()) {
			auto& lastBatch = batches.back();
			auto& objects = lastBatch->GetObjects();
			if (!objects.empty()) {
				if (auto mesh = objects[0]->GetComponent<Mesh>())
					mesh->ReleaseUploadBuffers();
			}
		}

		completedTasks++;

		float progress = (float)completedTasks / totalTasks;

		auto controller = ENGINE.GetUIManager()->GetController<LoadingSceneUIController>(SceneType::Loading);
		if (controller) controller->SetProgress(progress);
	}
	else
	{
		coreRef->SetLoadingMode(false);
	}
}

void LoadingScene::RenderSceneDeferred()
{
}

void LoadingScene::RenderSceneForward()
{
}

void LoadingScene::RenderSceneShadow()
{
}

void LoadingScene::RenderSceneEffects()
{
}

void LoadingScene::RequestSceneChange()
{
}

void LoadingScene::LoadSelectGameResources()
{
	auto controller = ENGINE.GetUIManager()->GetController<LoadingSceneUIController>(SceneType::Loading);
	if (controller) controller->SetProgress(1.0f);
	coreRef->SetLoadingMode(true);
}

void LoadingScene::LoadTownGameResources()
{
	auto controller = ENGINE.GetUIManager()->GetController<LoadingSceneUIController>(SceneType::Loading);
	if (controller) controller->SetProgress(1.0f);
	coreRef->SetLoadingMode(true);
}

void LoadingScene::LoadMainGameResources()
{
	InstanceLoader mapLoader;
	mapLoader.Load(L"../Assets/FBXModel/CastleMap/MapInstanceData.txt", L"../Assets/FBXModel/VillageMap/CullingData.txt");

	for (const auto& [modelName, instanceData] : mapLoader.GetAllData()) {
		if (instanceData.empty()) continue;

		wstring path = L"../Assets/FBXModel/CastleMap/" + wstring(modelName.begin(), modelName.end());
		if (!filesystem::exists(path + L"_0.mesh")) continue;

		loadTasks.push([this, path, instanceData]() {
			CreateAndBatchObjects(path, instanceData, sceneBatches[SceneType::MainGame]);
			});
	}

	totalTasks = loadTasks.size();

	coreRef->SetLoadingMode(true);
}
