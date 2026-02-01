#include "pch.h"
#include "ImGuiManager.h"
#include "DX12Core.h"
#include "Timer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "MainCharacter.h"
#include "Animator.h"

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
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
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

    // ���� �����
    if (showPerformance)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 120), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Performance", &showPerformance))
        {
            ImGui::Text("FPS: %d", TIMER.GetFps());
            ImGui::Text("Delta Time: %.3f ms", TIMER.GetDeltaTime() * 1000.0f);

            ImGui::Separator();
            ImGui::Checkbox("Light Editor", &showLightEditor);
            ImGui::Checkbox("Demo Window", &showDemoWindow);
        }
        ImGui::End();
    }

    // === ����Ʈ ������ �߰� ===
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
                ImGui::SliderFloat3("Direction##For", &forward.direction.x, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##For", &forward.color.x);
                ImGui::SliderFloat("Intensity##For", &forward.intensity, 0.0f, 2.0f);
            }

            // Main Directional Light (Deferred)
            if (ImGui::CollapsingHeader("Main Directional 1", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat3("Direction##Dir1", &deferred.lights[0].position.x, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##Dir1", &deferred.lights[0].color.x);
                ImGui::SliderFloat("Intensity##Dir1", &deferred.lights[0].intensity, 0.0f, 2.0f);
            }

            if (ImGui::CollapsingHeader("Main Directional 2", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat3("Direction##Dir2", &deferred.lights[1].position.x, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##Dir2", &deferred.lights[1].color.x);
                ImGui::SliderFloat("Intensity##Dir2", &deferred.lights[1].intensity, 0.0f, 2.0f);
            }

            if (ImGui::CollapsingHeader("Point Light 1", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat3("Position##Po1", &deferred.lights[2].position.x, -100.0f, 100.0f);
                ImGui::SliderFloat("Range##Po1", &deferred.lights[2].range, 1.0f, 3000.0f);
                ImGui::ColorEdit3("Color##Po1", &deferred.lights[2].color.x);
                ImGui::SliderFloat("Intensity##Po1", &deferred.lights[2].intensity, 0.0f, 2.0f);
            }

            // ������Ʈ
            coreRef->UpdateLights();
        }
        ImGui::End();
    }

    if (showAnimationEditor && myPlayer)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 270), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 380), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Animation Editor", &showAnimationEditor))
        {
            if (auto animator = myPlayer->GetComponent<Animator>())
            {
                float speed = animator->GetAnimationSpeed();
                if (ImGui::SliderFloat("Speed##myPlayer", &speed, 0.1f, 10.0f))
                    animator->SetAnimationSpeed(speed);
            }
        }
        ImGui::End();
    }


    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}