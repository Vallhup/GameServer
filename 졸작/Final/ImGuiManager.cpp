#include "pch.h"
#include "ImGuiManager.h"
#include "DX12Core.h"
#include "Timer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "MainCharacter.h"
#include "Animator.h"
#include "LightManager.h"
#include "SSAO.h"
#include "SkyBox.h"
#include "Camera.h"

void ImGuiManager::Initialize(HWND hwnd, DX12Core& core)
{
    coreRef = &core;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 2;
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
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    ImGui::Render();

    // 아무것도 그릴게 없으면 렌더 스킵
    if (!enabled && !showLoginWindow) return;

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
            ImGui::Checkbox("SSAO Editor", &showSsaoEditor);
            ImGui::Checkbox("Skybox Editor", &showSkyboxEditor);
            ImGui::Checkbox("LUT Presets", &showLutPresets);
            ImGui::Checkbox("Demo Window", &showDemoWindow);
        }
        ImGui::End();
    }

    if (showSsaoEditor && coreRef)
    {
        ImGui::SetNextWindowPos(ImVec2(270, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 130), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("SSAO Editor", &showSsaoEditor))
        {
            auto ssao = coreRef->GetSsaoMgr();
            if (ssao)
            {
                float radius = ssao->GetSamplingRadius();
                float bias = ssao->GetSsaoBias();

                bool changed = false;
                changed |= ImGui::SliderFloat("Radius", &radius, 0.1f, 5.0f);
                changed |= ImGui::SliderFloat("Bias", &bias, 0.001f, 0.5f);

                if (changed)
                {
                    ssao->SetSamplingRadius(radius);
                    ssao->SetSsaoBias(bias);
                    ssao->UpdateConstants();
                }
            }
        }
        ImGui::End();
    }

    if (showSkyboxEditor && skyBox)
    {
        ImGui::SetNextWindowPos(ImVec2(560, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 150), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Skybox Editor", &showSkyboxEditor))
        {
            auto& constants = skyBox->GetConstants();

            bool changed = false;
            changed |= ImGui::ColorEdit3("Tint Color", &constants.skyTintColor.x);
            changed |= ImGui::SliderFloat("Exposure", &constants.skyExposure, 0.1f, 3.0f);
            changed |= ImGui::SliderFloat("Saturation", &constants.skySaturation, 0.0f, 2.0f);

            if (changed)
            {
                skyBox->UpdateConstants();
            }

            if (ImGui::Button("Reset"))
            {
                constants.skyTintColor = { 1.0f, 1.0f, 1.0f };
                constants.skyExposure = 1.0f;
                constants.skySaturation = 1.0f;
                skyBox->UpdateConstants();
            }
        }
        ImGui::End();
    }

    if (showLightEditor && coreRef)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 140), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Light Editor", &showLightEditor))
        {
            auto& forward = coreRef->GetLightMgr()->GetForwardLightData();
            auto& deferred = coreRef->GetLightMgr()->GetDeferredLightData();

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

            coreRef->GetLightMgr()->UpdateLights();
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

    if (showLutPresets && camera)
    {
        ImGui::SetNextWindowPos(ImVec2(850, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 350), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("LUT Presets", &showLutPresets))
        {
            ImGui::Text("Current: LUT %d, Sat %.2f", camera->GetLutIndex(), camera->GetSaturation());
            ImGui::Separator();

            for (int i = 0; i < 8; ++i)
            {
                ImGui::PushID(i);

                char label[16];
                sprintf_s(label, "Preset %d", i + 1);

                if (ImGui::CollapsingHeader(label))
                {
                    int lutIdx = static_cast<int>(lutPresets[i].lutIndex);
                    if (ImGui::SliderInt("LUT Index", &lutIdx, 0, 219))
                        lutPresets[i].lutIndex = static_cast<UINT>(lutIdx);

                    ImGui::SliderFloat("Saturation", &lutPresets[i].saturation, 0.0f, 2.0f);

                    if (ImGui::Button("Apply"))
                    {
                        camera->SetLutPreset(lutPresets[i].lutIndex, lutPresets[i].saturation);
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Save Current"))
                    {
                        lutPresets[i].lutIndex = camera->GetLutIndex();
                        lutPresets[i].saturation = camera->GetSaturation();
                    }
                }

                ImGui::PopID();
            }
        }
        ImGui::End();
    }

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

void ImGuiManager::DrawLoginUI()
{
    if (!showLoginWindow) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImVec2 windowSize(350, 180);

    ImGui::SetNextWindowPos(ImVec2(center.x - windowSize.x * 0.5f, center.y - windowSize.y * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    // Style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 15));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));  

    // Color
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.95f));           // 어두운 배경
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));             // 타이틀바
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));       // 타이틀바 활성
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.30f, 0.30f, 0.35f, 1.0f));             // 입력창 배경
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.20f, 0.25f, 1.0f));              // 버튼
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.28f, 0.34f, 1.0f));       // 버튼 호버
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.16f, 0.20f, 1.0f));        // 버튼 클릭

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Login", &showLoginWindow, flags))
    {
        float contentWidth = ImGui::GetContentRegionAvail().x;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        ImGui::SetNextItemWidth(contentWidth);
        ImGui::InputTextWithHint("##id", "ID", loginId, 64);

        ImGui::Spacing();

        ImGui::SetNextItemWidth(contentWidth);
        ImGui::InputTextWithHint("##pw", "Password", loginPw, 64, ImGuiInputTextFlags_Password);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

        float buttonWidth = 100;
        float spacing = 20;
        float totalWidth = buttonWidth * 2 + spacing;
        ImGui::SetCursorPosX((windowSize.x - totalWidth) * 0.5f);

        if (ImGui::Button("Login", ImVec2(buttonWidth, 30)))
        {
            OutputDebugStringA("Login attempted!\n");
            OutputDebugStringA(("ID: " + string(loginId) + "\n").c_str());
            OutputDebugStringA(("Password: " + string(loginPw) + "\n").c_str());

            loginSuccess = true;
            showLoginWindow = false;

            memset(loginId, 0, sizeof(loginId));
            memset(loginPw, 0, sizeof(loginPw));
        }

        ImGui::SameLine(0, spacing);

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30)))
        {
            showLoginWindow = false;
            memset(loginId, 0, sizeof(loginId));
            memset(loginPw, 0, sizeof(loginPw));
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(4);
}
