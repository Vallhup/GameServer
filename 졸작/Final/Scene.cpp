#include "pch.h"
#include "Scene.h"
#include "DX12Graphics.h"
#include "UploadBuffer.h"

void Scene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    InitializeLogic(device, cmdList);
}

void Scene::Update(const float deltaTime)
{
    UpdateScene(deltaTime);
}

void Scene::Render()
{
    RenderScene();
}