#include "pch.h"
#include "Scene.h"
#include "DX12Graphics.h"
#include "UploadBuffer.h"

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
    XMMATRIX matView = XMLoadFloat4x4(&mView);
    XMMATRIX matProj = XMLoadFloat4x4(&mProjection);
    matView = XMMatrixTranspose(matView);
    matProj = XMMatrixTranspose(matProj);
    GET(DX12Graphics).GetFrameCB()->CopyData(&matView, sizeof(XMMATRIX), 0);
    GET(DX12Graphics).GetFrameCB()->CopyData(&matProj, sizeof(XMMATRIX), sizeof(XMMATRIX));

    UINT objectCounter = 0;
    GET(Graphics).DrawObjectsToWorld(GetWorld(), mViewPort, XMMatrixIdentity(), objectCounter);
}