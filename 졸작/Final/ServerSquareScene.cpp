#include "pch.h"
#include "ServerSquareScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Camera.h"

ServerSquareScene::~ServerSquareScene() = default;

void ServerSquareScene::Release()
{
}

void ServerSquareScene::Reset()
{
	Material::Cleanup();
	OutputDebugStringA("ServerSquareScene Data has been deleted!! \n----------------------------------------\n");
}

const float* ServerSquareScene::GetBackgroundColor()
{
	return Colors::Pink;
}

void ServerSquareScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nServerSquareScene Data has been created!! \n");
}

void ServerSquareScene::UpdateScene(const float deltaTime)
{
}

void ServerSquareScene::RenderSceneDeferred()
{
}

void ServerSquareScene::RenderSceneForward()
{

}

void ServerSquareScene::RenderSceneEffects()
{
}

int ServerSquareScene::GetSceneWidth() const
{
	return 0;
}

void ServerSquareScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::MainGame);
	}
}
