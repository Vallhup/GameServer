#pragma once
#include "Singleton.h"

class DX12Core;
class MainCharacter;
class SkyBox;

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
    MainCharacter* myPlayer = nullptr;
    SkyBox* skyBox = nullptr;

    bool showLoginWindow = false;
    bool loginSuccess = false;
    char loginId[64] = "";
    char loginPw[64] = "";
};