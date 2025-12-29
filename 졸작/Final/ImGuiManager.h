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

    bool IsEnabled() const { return mEnabled; }
    void SetEnabled(bool enabled) { mEnabled = enabled; }

private:
    ImGuiManager() = default;
    ~ImGuiManager() = default;

    ComPtr<ID3D12DescriptorHeap> mSrvHeap;
    DX12Core* mCore = nullptr;
    bool mEnabled = true;
    bool mShowDemoWindow = false;
    bool mShowPerformance = true;
};