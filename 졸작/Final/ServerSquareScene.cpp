#include "pch.h"
#include "ServerSquareScene.h"
#include "Input.h"
#include "SceneManager.h"

ServerSquareScene::~ServerSquareScene() = default;

void ServerSquareScene::Release()
{
}

void ServerSquareScene::Reset()
{
}

const float* ServerSquareScene::GetBackgroundColor()
{
	return Colors::Pink;
}

void ServerSquareScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
}

void ServerSquareScene::UpdateScene(const float deltaTime)
{
	if (GET(Input).GetKeyDown(VK_TAB))
		GET(SceneManager).ChangeScene(SceneType::MainGame);
}

void ServerSquareScene::RenderScene()
{
}

int ServerSquareScene::GetSceneWidth() const
{
	return 0;
}
