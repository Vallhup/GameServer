#include "pch.h"
#include "LoadingScene.h"
#include "Material.h"
#include "InstanceLoader.h"
#include "Engine.h"
#include "UIManager.h"
#include "LoadingSceneUIController.h"
#include "DX12Core.h"

LoadingScene::~LoadingScene() = default;

void LoadingScene::Release()
{
}

void LoadingScene::Reset()
{
	Material::ReleaseUploadBuffers();
	OutputDebugStringA("LoadingScene Data has been deleted!! \n----------------------------------------\n");
}

void LoadingScene::InitializeSceneObjectPools()
{
}

void LoadingScene::InitializeLogic()
{
	InstanceLoader mapLoader;
      mapLoader.Load(L"../Assets/FBXModel/Map/MapInstanceData.txt");

      for (const auto& [modelName, instanceData] : mapLoader.GetAllData()) {
          if (instanceData.empty()) continue;

          wstring path = L"../Assets/FBXModel/Map/" + wstring(modelName.begin(), modelName.end());
          if (!filesystem::exists(path + L"_0.mesh")) continue;

          loadTasks.push([this, path, instanceData]() {
              CreateAndBatchObjects(path, instanceData, instancingBatches);
          });
      }

	totalTasks = loadTasks.size();

	coreRef->SetLoadingMode(true);
}

void LoadingScene::UpdateScene(const float deltaTime)
{
	if (!loadTasks.empty()) {
		loadTasks.front()();
		loadTasks.pop();

		coreRef->ExecuteLoadingCommands();

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
