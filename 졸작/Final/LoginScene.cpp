#include "pch.h"
#include "LoginScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "Material.h"

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

void LoginScene::InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	OutputDebugStringA("----------------------------------------\nLoginScene Data has been created!! \n");
}

void LoginScene::UpdateScene(const float deltaTime)
{
	if (GET(Input).GetKeyDown(VK_TAB))
		GET(SceneManager).RequestSceneChange(SceneType::ServerSquare);
}

void LoginScene::RenderScene()
{
}

int LoginScene::GetSceneWidth() const
{
	return 0;
}
