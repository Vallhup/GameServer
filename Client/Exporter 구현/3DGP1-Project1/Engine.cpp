#include "pch.h"
#include "Engine.h"
#include "Timer.h"
#include "SceneManager.h"
#include "StartScene.h"
#include "MenuScene.h"
#include "RCScene.h"
#include "TGScene.h"
#include "Device.h"
#include "SwapChain.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "Shader.h"
#include "UploadBuffer.h"
#include "DepthStencilView.h"
#include "Indexes.h"
#include "DX12Graphics.h"
#include "Input.h"
#include "Importer.h"

Engine& Engine::Get()
{
    static Engine engine;
    return engine;
}

void Engine::Initialize(HWND hwnd)
{
    mHwnd = hwnd;

    viewport = { 0, 0, static_cast<FLOAT>(WinSize.x), static_cast<FLOAT>(WinSize.y), 0.0f, 1.0f };
    scissorRect = CD3DX12_RECT(0, 0, WinSize.x, WinSize.y);

    GET(DX12Graphics).Initialize(mHwnd);

    // 임시 확인용
    //vector<Vertex> vertices =
    //{
    //    { XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(0,0) },
    //    { XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(1,1) },
    //    { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT2(0,1) }
    //};
    //vector<UINT> indices = { 0, 1, 2 };

    //// 2. 커맨드리스트 받아옴
    //ID3D12GraphicsCommandList* cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();

    //// 3. 메시 초기화
    //mesh = make_unique<VertexIndexBuffer>();
    //mesh->Initialize(GET(DX12Graphics).GetDevice()->GetDevice().Get(), cmdList, vertices, indices);

    vector<Vertex> vertices;
    vector<UINT> indices;

    Importer loader;
    // 파일 읽기 (미분리)
    /*if (loader.Load(L"../AssetsBin/Dragon.bin", vertices, indices))
    {
        mesh = make_unique<VertexIndexBuffer>();
        mesh->Initialize(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), vertices, indices);
    }*/

    // 파일 읽기 (분리)
    if (loader.LoadSeparated(L"../AssetsBin/Strut Walking", vertices, indices))
    {
        mesh = make_unique<VertexIndexBuffer>();
        mesh->Initialize(GET(DX12Graphics).GetDevice()->GetDevice().Get(), GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get(), vertices, indices);
    }

    GET(DX12Graphics).FlushCommandQueue();
}

void Engine::Update(const float deltaTime)
{
    const float moveSpeed = 50.0f * deltaTime;
    const float rotSpeed = XMConvertToRadians(60.0f) * deltaTime;

    Input& input = GET(Input);

    if (input.GetKey(VK_LEFT))
        mCameraRot.y -= rotSpeed;
    if (input.GetKey(VK_RIGHT))
        mCameraRot.y += rotSpeed;
    if (input.GetKey(VK_UP))
        mCameraRot.x -= rotSpeed;
    if (input.GetKey(VK_DOWN))
        mCameraRot.x += rotSpeed;

    XMMATRIX rot = XMMatrixRotationRollPitchYaw(mCameraRot.x, mCameraRot.y, mCameraRot.z);
    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rot);  
    XMVECTOR move = XMVectorZero();

    if (input.GetKey('W'))
        move += forward;
    if (input.GetKey('S'))
        move -= forward;
    if (input.GetKey('D'))
        move += right;
    if (input.GetKey('A'))
        move -= right;

    move = XMVector3Normalize(move) * moveSpeed;

    XMVECTOR camPos = XMLoadFloat3(&mCameraPos);
    camPos = camPos + move;
    XMStoreFloat3(&mCameraPos, camPos);
}

void Engine::Render(const float deltaTime)
{
    GET(DX12Graphics).RenderBegin(viewport, scissorRect);
    
    auto* cmdList = GET(DX12Graphics).GetCmdQueue()->GetCmdList().Get();
    cmdList->SetGraphicsRootSignature(GET(DX12Graphics).GetRootSig()->Get());
    cmdList->SetPipelineState(GET(DX12Graphics).GetShader()->GetOpaquePSO());

    XMMATRIX rot = XMMatrixRotationRollPitchYaw(mCameraRot.x, mCameraRot.y, mCameraRot.z);
    XMVECTOR pos = XMLoadFloat3(&mCameraPos);
    XMVECTOR look = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rot);
    XMMATRIX view = XMMatrixLookAtLH(pos, XMVectorAdd(pos, look), up);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), static_cast<float>(WinSize.x) / WinSize.y, 0.01f, 1000.0f);
    XMMATRIX matview = XMMatrixTranspose(view);
    XMMATRIX matproj = XMMatrixTranspose(proj);

    GET(DX12Graphics).GetFrameCB()->CopyData(&matview, sizeof(XMMATRIX), 0);
    GET(DX12Graphics).GetFrameCB()->CopyData(&matproj, sizeof(XMMATRIX), sizeof(XMMATRIX));

    cmdList->SetGraphicsRootConstantBufferView(0, GET(DX12Graphics).GetFrameCB()->GetGPUVirtualAddress());

    mesh->Bind(cmdList);
    mesh->Draw(cmdList);

    GET(DX12Graphics).RenderEnd();

    ShowFps();
}

void Engine::Shutdown()
{

}

void Engine::ShowFps()
{
    UINT32 fps = GET(Timer).GetFps();
    WCHAR text[100] = L"";
    wsprintf(text, L"과제2(2019180051 이정호)         FPS : %d", fps);
    SetWindowText(mHwnd, text);
}