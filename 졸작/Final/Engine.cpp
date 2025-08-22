#include "pch.h"
#include "Engine.h"
#include "DX12Core.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "Timer.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexIndexBuffer.h"
#include "Importer.h"
#include "Camera.h"

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

    graphics = make_unique<DX12Core>();
    graphics->Initialize(mHwnd);

    sManager = make_unique<SceneManager>();
    sManager->Initialize(*graphics);

    nManager = make_unique<NetworkManager>();
    nManager->Initialize();

    graphics->FlushCommandQueue();
}

void Engine::Update(const float deltaTime)
{
    sManager->ProcessPendingSceneChange(*graphics);
    sManager->Update(deltaTime);

    nManager->Update();
}

void Engine::Render()
{
    graphics->RenderBegin(viewport, scissorRect);
    
    sManager->Render();

    graphics->RenderEnd();

    ShowFps();
}

void Engine::Shutdown()
{
    if (sManager && sManager->GetCurrentScene())
    {
        auto camera = sManager->GetCurrentScene()->GetCamera();
        if (camera)
        {
            camera->ReleaseMouse();
            OutputDebugStringA("Mouse Released!! \n");
        }
    }

    if (graphics)
        graphics->GetSwapChain()->SetFullscreenState(FALSE, nullptr);
    sManager->Release();

    nManager->Release();
}

void Engine::ShowFps()
{
    UINT32 fps = GET(Timer).GetFps();
    WCHAR text[100] = L"";
    wsprintf(text, L"Final      FPS: %d", fps);
    SetWindowText(mHwnd, text);
}

void Engine::TestFBXImport()
{
    Importer importer;

    if (importer.LoadModel(L"../FBXOutput/Dragon"))
    {
        OutputDebugStringA("=== FBX Import Success! ===\n");

        const MeshData& mesh = importer.GetMesh();
        string msg = "Vertices: " + to_string(mesh.vertices.size()) +
            ", Indices: " + to_string(mesh.indices.size()) + "\n";
        OutputDebugStringA(msg.c_str());

        if (importer.HasAnimation()) {
            const auto& anims = importer.GetAnimations();
            string animMsg = "Animations: " + to_string(anims.size()) + "\n";
            OutputDebugStringA(animMsg.c_str());
        }
    }
    else {
        OutputDebugStringA("FBX Import Failed!\n");
    }
}