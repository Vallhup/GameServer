#pragma once

enum class SceneType;
class Device;
class SwapChain;
class CommandQueue;
class RootSignature;
class Shader;
class UploadBuffer;
class VertexIndexBuffer;
class DepthStencilBuffer;

class Engine
{
public:
    static Engine& Get();

    void Initialize(HWND hwnd);
    void Update(const float deltaTime);  
    void Render(const float deltaTime);
    void Shutdown();  
    void ShowFps();

    HWND GetHWND() const { return mHwnd; }

private:
    HWND mHwnd = nullptr;

    D3D12_VIEWPORT	viewport = {};
    D3D12_RECT		scissorRect = {};
};