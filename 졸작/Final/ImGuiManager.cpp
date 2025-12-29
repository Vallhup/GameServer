#include "pch.h"
#include "ImGuiManager.h"
#include "DX12Core.h"
#include "Timer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

ImGuiManager& ImGuiManager::Get()
{
    static ImGuiManager instance;
    return instance;
}

void ImGuiManager::Initialize(HWND hwnd, DX12Core& core)
{
    mCore = &core;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = core.GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mSrvHeap));
    MASSERT(SUCCEEDED(hr), "Failed to create ImGui SRV heap");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(
        core.GetDevice(),
        SWAP_CHAIN_BUFFER_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        mSrvHeap.Get(),
        mSrvHeap->GetCPUDescriptorHandleForHeapStart(),
        mSrvHeap->GetGPUDescriptorHandleForHeapStart()
    );

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ImGui_ImplDX12_CreateDeviceObjects();

    OutputDebugStringA("ImGui initialized!\n");
}

void ImGuiManager::BeginFrame()
{
    if (!mEnabled) return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    if (!mEnabled) return;

    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { mSrvHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiManager::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    mSrvHeap.Reset();

    OutputDebugStringA("ImGui shutdown!\n");
}

void ImGuiManager::DrawDebugUI()
{
    if (!mEnabled) return;

    if (mShowPerformance)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Performance", &mShowPerformance))
        {
            ImGui::Text("FPS: %d", GET(Timer).GetFps());
            ImGui::Text("Delta Time: %.3f ms", GET(Timer).GetDeltaTime() * 1000.0f);
        }
        ImGui::End();
    }

    if (mShowDemoWindow)
    {
        ImGui::ShowDemoWindow(&mShowDemoWindow);
    }
}