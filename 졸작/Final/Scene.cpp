#include "pch.h"
#include "Scene.h"
#include "DX12Graphics.h"
#include "UploadBuffer.h"
#include "Material.h"

void Scene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    Material::InitializeBindlessSystem(device);

    InitializeLogic(device, cmdList);

    Material::UpdateMaterialBuffer();

    GET(DX12Graphics).FlushCommandQueue();  
    GET(DX12Graphics).ResetCommandQueue();

    Material::ReleaseUploadBuffers();
}

void Scene::Update(const float deltaTime)
{
    UpdateScene(deltaTime);
}

void Scene::Render()
{
    RenderScene();
}