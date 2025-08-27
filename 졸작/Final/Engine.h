#pragma once

class DX12Core;
class SceneManager;
class NetworkManager;

class Engine
{
public:
    static Engine& Get();

    void Initialize(HWND hwnd);
    void Update(const float deltaTime);  
    void Render();
    void Shutdown();  
    void ShowFps();
    void TestFBXImport();

    HWND GetHWND() const { return mHwnd; }

public:
    // Server Test
    SceneManager* GetSceneManager() { return sManager.get(); }

private:
    HWND mHwnd = nullptr;

    D3D12_VIEWPORT	viewport = {};
    D3D12_RECT		scissorRect = {};

    unique_ptr<DX12Core> graphics;
    unique_ptr<SceneManager> sManager;
    unique_ptr<NetworkManager> nManager;
};