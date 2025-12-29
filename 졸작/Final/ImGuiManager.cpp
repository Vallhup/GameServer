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
    coreRef = &core;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = core.GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap));
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
        srvHeap.Get(),
        srvHeap->GetCPUDescriptorHandleForHeapStart(),
        srvHeap->GetGPUDescriptorHandleForHeapStart()
    );

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ImGui_ImplDX12_CreateDeviceObjects();

    OutputDebugStringA("ImGui initialized!\n");
}

void ImGuiManager::BeginFrame()
{
    if (!enabled) return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    if (!enabled) return;

    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { srvHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiManager::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    srvHeap.Reset();

    OutputDebugStringA("ImGui shutdown!\n");
}

void ImGuiManager::DrawDebugUI()
{
    if (!enabled) return;

    // 성능 모니터
    if (showPerformance)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 120), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Performance", &showPerformance))
        {
            ImGui::Text("FPS: %d", GET(Timer).GetFps());
            ImGui::Text("Delta Time: %.3f ms", GET(Timer).GetDeltaTime() * 1000.0f);

            ImGui::Separator();
            ImGui::Checkbox("Light Editor", &showLightEditor);
            ImGui::Checkbox("Demo Window", &showDemoWindow);
        }
        ImGui::End();
    }

    // === 라이트 에디터 추가 ===
    if (showLightEditor && coreRef)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 140), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Light Editor", &showLightEditor))
        {
            auto& forward = coreRef->GetForwardLightData();
            auto& deferred = coreRef->GetDeferredLightData();

            // Forward Light
            if (ImGui::CollapsingHeader("Forward Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat3("Direction##F", &forward.direction.x, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##F", &forward.color.x);
                ImGui::SliderFloat("Intensity##F", &forward.intensity, 0.0f, 2.0f);
            }

            // Main Directional Light (Deferred)
            if (ImGui::CollapsingHeader("Main Directional", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat3("Direction##D0", &deferred.lights[0].position.x, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##D0", &deferred.lights[0].color.x);
                ImGui::SliderFloat("Intensity##D0", &deferred.lights[0].intensity, 0.0f, 2.0f);
            }

            // 업데이트
            coreRef->UpdateLights();
        }
        ImGui::End();
    }

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}