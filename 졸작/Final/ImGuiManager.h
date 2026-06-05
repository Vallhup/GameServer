#pragma once
#include "Singleton.h"

class DX12Core;
class MainCharacter;
class SkyBox;
class Camera;
struct ImFont;

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
    void Render();
    void EndFrame(ID3D12GraphicsCommandList* cmdList);
    void Shutdown();

    void DrawDebugUI();
    void DrawLoginUI();
    void DrawSettingsUI();
    void DrawFpsOverlay();           
    void ApplyFullscreen(bool fs);   

    void ShowSettingsWindow() { showSettingsWindow = true; settingsPage = 0; backRequested = false; settingsJustOpened = true; }
    void HideSettingsWindow() { showSettingsWindow = false; }
    bool IsSettingsOpen() const { return showSettingsWindow; }
    bool ConsumeSettingsBack() { bool b = backRequested; backRequested = false; return b; }   
    int  GetFrameLimitFps() const { return frameLimitFps; }   

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool in) { enabled = in; }

    bool IsSsaoEnabled() const { return ssaoEnabled; }          
    void SetSsaoEnabled(bool in) { ssaoEnabled = in; }
    void SetMyPlayer(MainCharacter* player) { myPlayer = player; }
    void SetSkyBox(SkyBox* sky) { skyBox = sky; }
    void SetCamera(Camera* cam) { camera = cam; }
    void SetLoginSuccess(bool canLogin) { loginSuccess = canLogin; }

    void ShowLoginWindow() { showLoginWindow = true; }
    bool IsLoginSuccess() const { return loginSuccess; }
    void ResetLoginSuccess() { loginSuccess = false; }

private:
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    DX12Core* coreRef = nullptr;
    HWND windowHandle = nullptr;
    bool enabled = false;
    bool showDemoWindow = false;
    bool showPerformance = true;
    bool showLightEditor = true;
    bool showSsaoEditor = true;
    bool showSkyboxEditor = true;
    bool showVolumetricFogEditor = true;
    bool showShadowEditor = true;

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

    ImFont* settingsFont = nullptr;    
    bool showLoginWindow = false;
    bool showSettingsWindow = false;
    bool backRequested = false;        
    bool settingsJustOpened = false;   
    int  settingsPage = 0;             // 0 = 시스템, 1 = 그래픽

    // 시스템 페이지 상태
    float masterVol  = 100.0f;   
    float bgmVol     = 30.0f;    
    float sfxVol     = 50.0f;    
    bool  fullscreen = true;     
    float brightness = 100.0f;   
    float saturation = 100.0f;   
    int   frameLimitIdx = 999;   
    int   frameLimitFps = 0;     
    float mouseSens  = 0.1f;     
    bool  showFpsCounter = false;

    // 그래픽 페이지 상태
    bool  ssaoEnabled   = true;  
    float shadowDarkness = 70.0f;
    float fogPickX = 0.0f, fogPickY = 0.0f;  
    bool loginSuccess = false;
    char loginId[64] = "";
    char loginPw[64] = "";
};