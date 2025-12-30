#pragma once

class DX12Core;

class ImGuiManager
{
public:
    static ImGuiManager& Get();

    void Initialize(HWND hwnd, DX12Core& core);
    void BeginFrame();
    void EndFrame(ID3D12GraphicsCommandList* cmdList);
    void Shutdown();

    void DrawDebugUI();

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool in) { enabled = in; }

private:
    ImGuiManager() = default;
    ~ImGuiManager() = default;

    ComPtr<ID3D12DescriptorHeap> srvHeap;
    DX12Core* coreRef = nullptr;
    bool enabled = true;
    bool showDemoWindow = false;
    bool showPerformance = true;
    bool showLightEditor = true;
};