#pragma once
#include "Singleton.h"

class DX12Core;
class MainCharacter;
class SkyBox;
class Camera;

struct LutPreset
{
    UINT lutIndex = 0;
    float saturation = 1.0f;
};

class ImGuiManager : public Singleton<ImGuiManager>
{
    friend class Singleton<ImGuiManager>;
    ImGuiManager() = default;
    ~ImGuiManager() = default;

public:
    void Initialize(HWND hwnd, DX12Core& core);
    void BeginFrame();
    void EndFrame(ID3D12GraphicsCommandList* cmdList);
    void Shutdown();

    void DrawDebugUI();
    void DrawLoginUI();

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool in) { enabled = in; }
    void SetMyPlayer(MainCharacter* player) { myPlayer = player; }
    void SetSkyBox(SkyBox* sky) { skyBox = sky; }
    void SetCamera(Camera* cam) { camera = cam; }
    void SetWaterDebugTexture(ID3D12Device* device, ID3D12Resource* texture);

    void ShowLoginWindow() { showLoginWindow = true; }
    bool IsLoginSuccess() const { return loginSuccess; }
    void ResetLoginSuccess() { loginSuccess = false; }

private:
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    DX12Core* coreRef = nullptr;
    bool enabled = false;
    bool showDemoWindow = false;
    bool showPerformance = true;
    bool showLightEditor = true;
    bool showSsaoEditor = true;
    bool showSkyboxEditor = true;

    bool showAnimationEditor = true;
    bool showLutPresets = true;

    MainCharacter* myPlayer = nullptr;
    SkyBox* skyBox = nullptr;
    Camera* camera = nullptr;

    LutPreset lutPresets[8] = {
        { 14, 1.05f },
        { 17, 2.05f },
        { 43, 1.55f },
        { 50, 1.05f },
        { 59, 2.05f },
        { 104, 1.05f },
        { 105, 1.05f },
        { 110, 1.05f }
    };

    bool showLoginWindow = false;
    bool loginSuccess = false;
    char loginId[64] = "";
    char loginPw[64] = "";

    D3D12_GPU_DESCRIPTOR_HANDLE waterDebugSRV = {};
    bool hasWaterDebug = false;
};