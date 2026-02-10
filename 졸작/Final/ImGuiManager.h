#pragma once
#include "Singleton.h"

class DX12Core;
class MainCharacter;

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

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool in) { enabled = in; }
    void SetMyPlayer(MainCharacter* player) { myPlayer = player; }

private:
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    DX12Core* coreRef = nullptr;
    bool enabled = false;
    bool showDemoWindow = false;
    bool showPerformance = true;
    bool showLightEditor = true;

    bool showAnimationEditor = true;
    MainCharacter* myPlayer = nullptr;
};