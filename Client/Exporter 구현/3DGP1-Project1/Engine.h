#pragma once
#include "VertexIndexBuffer.h"

enum class SceneType;
class Device;
class SwapChain;
class CommandQueue;
class RootSignature;
class Shader;
class UploadBuffer;
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

    unique_ptr<VertexIndexBuffer> mesh = {};

    //XMFLOAT3 mCameraPos = { 100.0f, 100.0f, -150.0f };   // 초기 카메라 위치
    XMFLOAT3 mCameraPos = { 0.0f, 0.0f, -2.0f };   // 초기 카메라 위치
    XMFLOAT3 mCameraRot = { 0.0f, 0.0f, 0.0f };
};