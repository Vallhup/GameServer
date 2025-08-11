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
	OutputDebugStringA("ServerSquareScene Data has been deleted!! \n----------------------------------------\n");
}

const float* ServerSquareScene::GetBackgroundColor()
{
	return Colors::Pink;
}

void ServerSquareScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	OutputDebugStringA("----------------------------------------\nServerSquareScene Data has been created!! \n");
}

void ServerSquareScene::UpdateScene(const float deltaTime)
{
	if (GET(Input).GetKeyDown(VK_TAB))
		GET(SceneManager).RequestSceneChange(SceneType::MainGame);
}

void ServerSquareScene::RenderScene()
{
}

int ServerSquareScene::GetSceneWidth() const
{
	return 0;
}
