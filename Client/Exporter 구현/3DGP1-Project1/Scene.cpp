#include "pch.h"
#include "Scene.h"

void Scene::Initialize(HWND hwnd, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    mhwnd = hwnd;
    InitializeProjection();
    InitializeLogic(device, cmdList);
}

void Scene::Update(const float deltaTime)
{
    GET(Graphics).SetView(mView, mCamera.GetTransform().GetPosition(), mCamera.GetTransform().GetLookDir());

    UpdateLogic(deltaTime);
}

void Scene::Render()
{
    UINT objectCounter = 0;
    GET(Graphics).DrawObjectsToWorld(GetWorld(), mView, mProjection, mViewPort, XMMatrixIdentity(), objectCounter);
}