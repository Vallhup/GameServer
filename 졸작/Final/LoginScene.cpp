#include "pch.h"
#include "LoginScene.h"
#include "DX12Core.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"
#include "Camera.h"

LoginScene::~LoginScene() = default;

void LoginScene::Release()
{
}

void LoginScene::Reset()
{
	Material::Cleanup();
	OutputDebugStringA("LoginScene Data has been deleted!! \n----------------------------------------\n");
}

const float* LoginScene::GetBackgroundColor()
{
	return Colors::MediumAquamarine;
}

void LoginScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nLoginScene Data has been created!! \n");
}

void LoginScene::UpdateScene(const float deltaTime)
{
}

void LoginScene::RenderSceneDeferred()
{
}

void LoginScene::RenderSceneForward()
{
}

int LoginScene::GetSceneWidth() const
{
	return 0;
}

void LoginScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::ServerSquare);
	}
}
