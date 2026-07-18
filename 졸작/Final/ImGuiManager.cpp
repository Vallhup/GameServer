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
#include "ShadowMappingManager.h"
#include "SSAO.h"
#include "BloomManager.h"
#include "SkyBox.h"
#include "Camera.h"
#include "Engine.h"
#include "NetworkManager.h"
#include "SoundManager.h"
#include "SwapChain.h"

void ImGuiManager::Initialize(HWND hwnd, DX12Core& core)
{
    coreRef = &core;
    windowHandle = hwnd;

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
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;   

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

    io.Fonts->AddFontFromFileTTF(
        "../Assets/UI/Fonts/malgunbd.ttf", 20.0f, nullptr,
        io.Fonts->GetGlyphRangesKorean());

    settingsFont = io.Fonts->AddFontFromFileTTF(
        "../Assets/UI/Fonts/malgunbd.ttf", 48.0f, nullptr,
        io.Fonts->GetGlyphRangesKorean());

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ImGui_ImplDX12_CreateDeviceObjects();

    ApplyFullscreen(fullscreen);

    masterVol = 100.0f;
    bgmVol = SOUND_MANAGER->GetBaseBGMVolume() * 100.0f;
    sfxVol = SOUND_MANAGER->GetBaseSFXVolume() * 100.0f;

    OutputDebugStringA("ImGui initialized!\n");
}

void ImGuiManager::BeginFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(WinSize.x), static_cast<float>(WinSize.y));
    ImGui::NewFrame();
}

void ImGuiManager::Render()
{
    DrawDebugUI();
    DrawLoginUI();
    DrawSettingsUI();
    DrawFpsOverlay();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    ImGui::Render();

    if (!enabled && !showLoginWindow && !showSettingsWindow && !showFpsCounter) return;

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
            ImGui::Checkbox("Volumetric Fog Editor", &showVolumetricFogEditor);
            ImGui::Checkbox("Shadow Editor", &showShadowEditor);
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
            auto* lts = coreRef->GetLightMgr()->GetLights();

            if (skyBox && ImGui::CollapsingHeader("Sun (Skybox)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& sun = skyBox->GetSun();
                ImGui::SliderFloat3("Direction##Sun", &sun.direction.x, -1.0f, 1.0f);
                ImGui::ColorEdit3("Color##Sun", &sun.color.x);
                ImGui::SliderFloat("Intensity##Sun", &sun.intensity, 0.0f, 5.0f);
            }

            coreRef->GetLightMgr()->UpdateLights();
        }
        ImGui::End();
    }

    if (showVolumetricFogEditor && coreRef)
    {
        ImGui::SetNextWindowPos(ImVec2(320, 140), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320, 380), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Volumetric Fog Editor", &showVolumetricFogEditor))
        {
            auto& vf = coreRef->GetVolumetricFogData();
            bool changed = false;

            if (ImGui::CollapsingHeader("Fog Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                changed |= ImGui::SliderFloat("Density", &vf.density, 0.001f, 0.1f, "%.4f");
                changed |= ImGui::SliderFloat("Scattering", &vf.scattering, 0.0f, 2.0f);
                changed |= ImGui::SliderFloat("Absorption", &vf.absorption, 0.0f, 1.0f);
            }

            if (ImGui::CollapsingHeader("Ray Marching", ImGuiTreeNodeFlags_DefaultOpen))
            {
                changed |= ImGui::SliderInt("Max Steps", &vf.maxSteps, 8, 64);
                changed |= ImGui::SliderFloat("Max Distance", &vf.maxDistance, 50.0f, 500.0f);
                changed |= ImGui::SliderFloat("Jitter Strength", &vf.jitterStrength, 0.0f, 1.0f);
            }

            if (ImGui::CollapsingHeader("Height Fog", ImGuiTreeNodeFlags_DefaultOpen))
            {
                changed |= ImGui::SliderFloat("Height Falloff", &vf.heightFalloff, 0.0001f, 0.01f, "%.4f");
                changed |= ImGui::SliderFloat("Ground Height", &vf.groundHeight, -10.0f, 50.0f);
            }

            if (ImGui::CollapsingHeader("Light Shaft", ImGuiTreeNodeFlags_DefaultOpen))
            {
                changed |= ImGui::SliderFloat("HG Anisotropy", &vf.hgAnisotropy, 0.0f, 0.99f);
                changed |= ImGui::ColorEdit3("Light Color", &vf.lightColor.x);
                changed |= ImGui::SliderFloat("Light Intensity", &vf.lightIntensity, 0.0f, 5.0f);
            }

            if (changed)
            {
                coreRef->UpdateVolumetricFog();
            }

            if (ImGui::Button("Reset to Default"))
            {
                vf.density = 0.02f;
                vf.scattering = 0.8f;
                vf.absorption = 0.1f;
                vf.hgAnisotropy = 0.6f;
                vf.maxSteps = 32;
                vf.maxDistance = 160.0f;
                vf.jitterStrength = 0.5f;
                vf.heightFalloff = 0.001f;
                vf.groundHeight = 3.0f;
                vf.lightColor = { 1.0f, 1.0f, 1.0f };
                vf.lightIntensity = 1.0f;
                coreRef->UpdateVolumetricFog();
            }
        }
        ImGui::End();
    }

    if (showShadowEditor && coreRef)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 140), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 100), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Shadow Editor", &showShadowEditor))
        {
            auto* sm = coreRef->GetShadowMgr();
            if (sm)
            {
                auto& cs = sm->GetCsmConstants();
                bool changed = false;
                changed |= ImGui::SliderFloat("Ambient Min (in Shadow)", &cs.shadowAmbientMin, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("Shadow Floor", &cs.shadowFloor, 0.0f, 1.0f);

                if (changed)
                {
                    sm->UploadCsmConstants();
                }

                ImGui::Separator();
                ImGui::TextUnformatted("Overhead (Indoor) Shadow");
                ImGui::SliderFloat("Shadow Strength", &cs.overheadStrength, 0.0f, 1.0f);
                ImGui::SliderFloat("Ambient Fill", &cs.overheadAmbientBoost, 1.0f, 4.0f);
                ImGui::SliderFloat3("Light Dir", &sm->GetOverheadLightDir().x, -1.0f, 1.0f);
                ImGui::SliderFloat("Half Size", &sm->GetOverheadHalfSize(), 10.0f, 120.0f);
                ImGui::SliderFloat("Height", &sm->GetOverheadHeight(), 20.0f, 200.0f);

                ImGui::Separator();
                ImGui::TextUnformatted("Point Light Static Shadow (Final)");

                if (ImGui::SliderFloat("Strength##point", &cs.pointShadowStrength, 0.0f, 1.0f))
                    sm->UploadCsmConstants();

                ImGui::SliderFloat("Near##point", &cs.pointShadowNear, 0.05f, 1.0f);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    sm->UploadCsmConstants();
                    sm->SetPointShadowBaked(false);   
                }
            }
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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 15));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));  

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.95f));           
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.08f, 0.10f, 1.0f));             
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));       
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.30f, 0.30f, 0.35f, 1.0f));             
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.20f, 0.25f, 1.0f));              
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.28f, 0.34f, 1.0f));       
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.16f, 0.20f, 1.0f));        

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

            NETWORK_MANAGER->SendLoginPacket(string(loginId), string(loginPw));
            OutputDebugStringA("CSLoginPacket has sent!!\n");

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

void ImGuiManager::DrawSettingsUI()
{
    if (!showSettingsWindow) return;

    if (settingsJustOpened)
    {
        if (camera) 
            saturation = clamp(camera->GetSaturation() * 100.0f, 1.0f, 200.0f);

        if (coreRef && coreRef->GetShadowMgr())
            shadowDarkness = clamp((1.0f - coreRef->GetShadowMgr()->GetCsmConstants().shadowAmbientMin) * 100.0f, 0.0f, 100.0f);

        settingsJustOpened = false;
    }

    auto toU8 = [](const wchar_t* w) -> string {
        int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        string s(len > 0 ? len - 1 : 0, '\0');

        if (len > 0) 
            WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), len, nullptr, nullptr);

        return s;
    };

    const string  title = toU8(L"환경설정");
    const string  pageName = toU8(settingsPage == 0 ? L"시스템" : L"그래픽");

    ImGuiIO& io = ImGui::GetIO();
    const float uiScale = io.DisplaySize.y / 1080.0f; 

    auto arrowBtn = [](const char* id, ImGuiDir dir) -> bool {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float sz = ImGui::GetFrameHeight();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, ImVec2(sz, sz));

        const bool clicked = ImGui::IsItemClicked();

        const ImU32 col = ImGui::IsItemActive()  ? IM_COL32(255, 240, 200, 255)   
                        : ImGui::IsItemHovered() ? IM_COL32(245, 226, 172, 255)   
                                                 : IM_COL32(196, 168, 108, 255);  

        const ImVec2 c = ImVec2(p.x + sz * 0.5f, p.y + sz * 0.5f);
        const float r = sz * 0.30f;

        ImVec2 tip, top, bot;

        if (dir == ImGuiDir_Left) 
        { 
            tip = { c.x - r, c.y }; 
            top = { c.x + r * 0.8f, c.y - r }; 
            bot = { c.x + r * 0.8f, c.y + r }; 
        }
        else 
        { 
            tip = { c.x + r, c.y }; 
            top = { c.x - r * 0.8f, c.y - r }; 
            bot = { c.x - r * 0.8f, c.y + r }; 
        }
        dl->AddTriangleFilled(tip, top, bot, col);

        return clicked;
    };

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.86f, 0.70f, 1.0f));   
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));    
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.75f, 0.45f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.75f, 0.45f, 0.45f));

    if (settingsFont) 
        ImGui::PushFont(settingsFont);

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.06f, io.DisplaySize.y * 0.09f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("##SettingsTitle", nullptr, flags | ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        ImGui::SetWindowFontScale(1.25f * uiScale);   
        ImGui::TextUnformatted(title.c_str());
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();

    const string back = toU8(L"BACK");
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.94f, io.DisplaySize.y * 0.09f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    if (ImGui::Begin("##SettingsBack", nullptr, flags | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetWindowFontScale(1.25f * uiScale);

        const ImVec2 p  = ImGui::GetCursorScreenPos();
        const ImVec2 ts = ImGui::CalcTextSize(back.c_str());

        ImGui::InvisibleButton("##backHit", ts);

        if (ImGui::IsItemClicked()) 
            backRequested = true;

        const ImU32 col = ImGui::IsItemActive()  ? IM_COL32(255, 240, 200, 255)
                        : ImGui::IsItemHovered() ? IM_COL32(245, 226, 172, 255)
                                                 : IM_COL32(196, 168, 108, 255);

        ImGui::GetWindowDrawList()->AddText(p, col, back.c_str());
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.11f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.35f, io.DisplaySize.y * 0.12f), ImGuiCond_Always);

    if (ImGui::Begin("##SettingsNav", nullptr, flags))
    {
        ImGui::SetWindowFontScale(0.95f * uiScale);   
        const float availW  = ImGui::GetContentRegionAvail().x;
        const float arrowSz = ImGui::GetFrameHeight();
        const float pageW   = ImGui::CalcTextSize(pageName.c_str()).x;
        const float textH   = ImGui::GetTextLineHeight();
        const float gap     = io.DisplaySize.x * 0.022f;
        const float total   = arrowSz + gap + pageW + gap + arrowSz;
        const float baseX   = ImGui::GetCursorPosX() + (availW - total) * 0.5f;
        const float baseY   = ImGui::GetCursorPosY();

        ImGui::SetCursorPos(ImVec2(baseX, baseY));

        if (arrowBtn("##PrevPage", ImGuiDir_Left))
            settingsPage = (settingsPage + 1) % 2;

        ImGui::SetCursorPos(ImVec2(baseX + arrowSz + gap, baseY + (arrowSz - textH) * 0.5f));
        ImGui::TextUnformatted(pageName.c_str());

        ImGui::SetCursorPos(ImVec2(baseX + arrowSz + gap + pageW + gap, baseY));

        if (arrowBtn("##NextPage", ImGuiDir_Right))
            settingsPage = (settingsPage + 1) % 2;

        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();

    const float contentH = (settingsPage == 0) ? 0.74f : 0.74f;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.20f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.60f, io.DisplaySize.y * contentH), ImGuiCond_Always);

    if (ImGui::Begin("##SettingsContent", nullptr, flags))
    {
        const float fullW = ImGui::GetContentRegionAvail().x;
        float colX  = fullW * 0.30f;    
        float ctrlW = fullW * 0.46f;    
        const float HDR   = 0.78f * uiScale;  
        const float BODY  = 0.62f * uiScale;  

        auto slider = [&](const char* id, float* v, float vmin, float vmax) -> bool {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float h  = ImGui::GetFrameHeight();
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton(id, ImVec2(ctrlW, h));
            const bool active = ImGui::IsItemActive();
            const bool hover  = ImGui::IsItemHovered();
            const float radius = h * 0.28f;
            const float cy = p.y + h * 0.5f;
            const float x0 = p.x + radius, x1 = p.x + ctrlW - radius;
            float t = (*v - vmin) / (vmax - vmin);
            t = t < 0 ? 0 : (t > 1 ? 1 : t);
            bool changed = false;

            if (active) {
                float nt = (ImGui::GetIO().MousePos.x - x0) / (x1 - x0);
                nt = nt < 0 ? 0 : (nt > 1 ? 1 : nt);
                float nv = vmin + nt * (vmax - vmin);
                
                if (nv != *v) { 
                    *v = nv; changed = true; 
                }

                t = nt;
            }

            const float hx = x0 + t * (x1 - x0);
            const float th = h * 0.08f;

            dl->AddLine(ImVec2(x0, cy), ImVec2(x1, cy), IM_COL32(74, 66, 50, 255), th);
            dl->AddLine(ImVec2(x0, cy), ImVec2(hx, cy), IM_COL32(212, 178, 116, 255), th);
            dl->AddCircleFilled(ImVec2(hx, cy), radius, (active || hover) ? IM_COL32(245, 226, 172, 255) : IM_COL32(222, 194, 128, 255), 24);

            return changed;
        };

        auto toggle = [&](const char* id, bool* b) -> bool {
            ImDrawList* dl = ImGui::GetWindowDrawList();

            const float h = ImGui::GetFrameHeight();
            const float w = h * 1.9f;

            const ImVec2 p = ImGui::GetCursorScreenPos();

            ImGui::InvisibleButton(id, ImVec2(w, h));

            bool changed = false;

            if (ImGui::IsItemClicked()) { 
                *b = !*b; changed = true; 
            }

            const float r = h * 0.5f;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), *b ? IM_COL32(150, 120, 60, 255) : IM_COL32(58, 52, 42, 255), r);

            float kx = *b ? (p.x + w - r) : (p.x + r);

            dl->AddCircleFilled(ImVec2(kx, p.y + r), r * 0.78f, IM_COL32(238, 222, 182, 255), 24);

            return changed;
        };

        auto header = [&](const wchar_t* t) {
            ImGui::SetWindowFontScale(HDR);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.74f, 0.46f, 1.0f));
            ImGui::TextUnformatted(toU8(t).c_str());
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight() * 0.18f));
            ImGui::SetWindowFontScale(BODY);
        };

        auto rowLabel = [&](const wchar_t* t) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(toU8(t).c_str());
            ImGui::SameLine(colX);
        };

        auto gap = [&]() { 
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight() * 0.50f)); 
        };

        if (settingsPage == 0)   
        {
            auto applyVol = [&]() {
                SOUND_MANAGER->SetBGMVolume((masterVol / 100.0f) * (bgmVol / 100.0f));
                SOUND_MANAGER->SetSFXVolume((masterVol / 100.0f) * (sfxVol / 100.0f));
            };

            header(L"사운드");
            rowLabel(L"마스터 볼륨"); 

            if (slider("##master", &masterVol, 0, 100)) 
                applyVol();

            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding(); 
            ImGui::Text("%.0f%%", masterVol); 
            gap();

            rowLabel(L"배경음");      

            if (slider("##bgm", &bgmVol, 0, 100)) 
                applyVol();

            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding(); 
            ImGui::Text("%.0f%%", bgmVol); 
            gap();

            rowLabel(L"효과음");      

            if (slider("##sfx", &sfxVol, 0, 100)) 
                applyVol();

            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding(); 
            ImGui::Text("%.0f%%", sfxVol); 
            gap();

            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight() * 0.35f));

            header(L"화면");

            rowLabel(L"전체화면"); 

            if (toggle("##fs", &fullscreen)) 
                ApplyFullscreen(fullscreen);

            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(toU8(fullscreen ? L"켜짐" : L"꺼짐").c_str()); 
            gap();

            rowLabel(L"프레임 제한");
            
            SwapChain* sc = coreRef ? coreRef->GetSwapChainMgr() : nullptr;
            const int native = sc ? sc->GetNativeRefresh() : 60;

            int fpsDesc[8]; 
            int n = 0;   

            for (UINT iv = 1; iv <= 8; iv *= 2)
            {
                int fps = native / (int)iv;

                if (iv > 1 && fps < 60) 
                    break;              

                fpsDesc[n++] = fps;
            }

            int  fpsOpt[8]; 
            int nOpt = 0;  

            for (int i = n - 1; i >= 0; --i) 
                fpsOpt[nOpt++] = fpsDesc[i];

            fpsOpt[nOpt++] = 0;      

            if (frameLimitIdx < 0 || frameLimitIdx >= nOpt) 
                frameLimitIdx = nOpt - 1;   

            if (arrowBtn("##fpsL", ImGuiDir_Left))  
                frameLimitIdx = (frameLimitIdx + nOpt - 1) % nOpt; 

            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding();

            if (fpsOpt[frameLimitIdx] == 0) 
                ImGui::TextUnformatted(toU8(L"무제한").c_str());
            else                            
                ImGui::Text("%d", fpsOpt[frameLimitIdx]);

            ImGui::SameLine(); 

            if (arrowBtn("##fpsR", ImGuiDir_Right)) 
                frameLimitIdx = (frameLimitIdx + 1) % nOpt;  

            frameLimitFps = fpsOpt[frameLimitIdx];   
            gap();

            rowLabel(L"FPS 표시"); 
            toggle("##fpscnt", &showFpsCounter);
            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(toU8(showFpsCounter ? L"켜짐" : L"꺼짐").c_str()); 
            gap();

            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight() * 0.35f));

            header(L"입력");
            rowLabel(L"마우스 감도");  

            if (slider("##sens", &mouseSens, 0.01f, 2.0f) && camera)
            {
                mouseSens = roundf(mouseSens * 100.0f) / 100.0f;   
                camera->SetMouseSensitivity(mouseSens);
            }

            ImGui::SameLine(); 
            ImGui::AlignTextToFramePadding(); 
            ImGui::Text("%.2f", mouseSens); 
            gap();

            ImGui::SetWindowFontScale(1.0f);
        }
        else
        {
            VolumetricFogConstants* vf = coreRef ? &coreRef->GetVolumetricFogData() : nullptr;
            ShadowMappingManager*   sm = coreRef ? coreRef->GetShadowMgr() : nullptr;

            const float colW = fullW * 0.46f;
            const ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

            auto section = [&]() {
                ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight() * 0.70f));
            };

            header(L"디스플레이");
            colX = fullW * 0.13f;
            ctrlW = fullW * 0.58f;
            rowLabel(L"밝기");

            if (slider("##bright", &brightness, 1, 200) && camera)
                camera->SetBrightness(brightness / 100.0f);

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%.0f%%", brightness);
            gap();

            rowLabel(L"채도");

            if (slider("##sat", &saturation, 1, 200) && camera)
                camera->SetSaturation(saturation / 100.0f);

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%.0f%%", saturation);
            gap();

            ImGui::SetWindowFontScale(1.0f);

            section();
            ImGui::Separator();
            section();

            ImGui::BeginChild("##gfxLeft", ImVec2(colW, 0.0f), false, childFlags);

            const float cw = ImGui::GetContentRegionAvail().x;
            colX = cw * 0.42f; ctrlW = cw * 0.40f;

            header(L"앰비언트 오클루전");
            rowLabel(L"SSAO");
            toggle("##ssao", &ssaoEnabled);
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(toU8(ssaoEnabled ? L"켜짐" : L"꺼짐").c_str());
            gap();
            section();

            header(L"그림자");
            rowLabel(L"어둡기");

            if (slider("##shadow", &shadowDarkness, 0, 100) && sm)
            {
                sm->GetCsmConstants().shadowAmbientMin = 1.0f - shadowDarkness / 100.0f;
                sm->UploadCsmConstants();
            }
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%.0f%%", shadowDarkness);
            gap();
            section();

            header(L"블룸");

            BloomManager* bloomMgr = coreRef ? coreRef->GetBloomMgr() : nullptr;
            float bloom = bloomMgr ? bloomMgr->GetIntensity() : 0.1f;
            rowLabel(L"강도");

            if (slider("##bloom", &bloom, 0.0f, 0.5f) && bloomMgr)
                bloomMgr->SetIntensity(bloom);

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%.2f", bloom);
            gap();

            ImGui::EndChild();

            ImGui::SameLine(0.0f, fullW * 0.06f);

            ImGui::BeginChild("##gfxRight", ImVec2(colW, 0.0f), false, childFlags);

            const float cwR = ImGui::GetContentRegionAvail().x;
            colX = cwR * 0.42f; ctrlW = cwR * 0.40f;
            auto vgap = [&]() {
                ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight() * 0.95f));
            };

            header(L"볼류메트릭");

            if (vf)
            {
                rowLabel(L"안개 밀도");

                if (slider("##fogden", &vf->density, 0.001f, 0.1f))
                    coreRef->UpdateVolumetricFog();

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%.3f", vf->density);
                vgap();

                rowLabel(L"안개 산란");

                if (slider("##fogsca", &vf->scattering, 0.0f, 2.0f))
                    coreRef->UpdateVolumetricFog();

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%.2f", vf->scattering);
                vgap();

                rowLabel(L"빛무리 색");

                const float sw = ImGui::GetFrameHeight();

                if (ImGui::ColorButton("##fogcol", ImVec4(vf->lightColor.x, vf->lightColor.y, vf->lightColor.z, 1.0f),
                    ImGuiColorEditFlags_NoTooltip, ImVec2(sw, sw)))
                {
                    fogPickX = ImGui::GetMousePos().x;
                    fogPickY = ImGui::GetMousePos().y;
                    ImGui::OpenPopup("##fogcolpick");
                }
                ImGui::SetNextWindowPos(ImVec2(fogPickX, fogPickY), ImGuiCond_Appearing);

                if (ImGui::BeginPopup("##fogcolpick"))
                {
                    ImGui::SetWindowFontScale(BODY);
                    ImGui::SetNextItemWidth(240.0f * uiScale);

                    if (ImGui::ColorPicker3("##pick", &vf->lightColor.x,
                        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel |
                        ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex))
                        coreRef->UpdateVolumetricFog();

                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::EndPopup();
                }

                vgap();

                rowLabel(L"빛무리 강도");

                if (slider("##fogint", &vf->lightIntensity, 0.0f, 5.0f))
                    coreRef->UpdateVolumetricFog();

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%.1f", vf->lightIntensity);
                vgap();

                rowLabel(L"레이마칭 품질");

                const int stepVals[3] = { 32, 64, 128 };
                const wchar_t* names[3] = { L"하", L"중", L"상" };
                int lvl = (vf->maxSteps >= 128) ? 2 : (vf->maxSteps >= 64 ? 1 : 0);

                if (arrowBtn("##rmL", ImGuiDir_Left))
                    lvl = (lvl + 2) % 3;

                ImGui::SameLine(); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(toU8(names[lvl]).c_str());
                ImGui::SameLine();

                if (arrowBtn("##rmR", ImGuiDir_Right))
                    lvl = (lvl + 1) % 3;

                if (stepVals[lvl] != vf->maxSteps) {
                    vf->maxSteps = stepVals[lvl];
                    coreRef->UpdateVolumetricFog();
                }
            }

            ImGui::EndChild();

            ImGui::SetWindowFontScale(1.0f);
        }
    }
    ImGui::End();

    if (settingsFont) 
        ImGui::PopFont();

    ImGui::PopStyleColor(4);
}

void ImGuiManager::DrawFpsOverlay()
{
    if (!showFpsCounter) return;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.99f, io.DisplaySize.y * 0.012f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    ImGuiWindowFlags f = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("##FpsOverlay", nullptr, f))
    {
        if (settingsFont) 
            ImGui::PushFont(settingsFont);

        ImGui::SetWindowFontScale(0.5f * (io.DisplaySize.y / 1080.0f));   
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.86f, 0.70f, 1.0f));
        ImGui::Text("FPS %d", TIMER.GetFps());
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);

        if (settingsFont) 
            ImGui::PopFont();
    }
    ImGui::End();
}

void ImGuiManager::ApplyFullscreen(bool fs)
{
    if (!windowHandle) return;

    HMONITOR mon = MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(mon, &mi);

    if (fs)
    {
        SetWindowLongPtr(windowHandle, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(windowHandle, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        SetWindowLongPtr(windowHandle, GWL_STYLE, style);

        RECT frame = { 0, 0, 0, 0 };
        AdjustWindowRect(&frame, style, FALSE);   
        const int frameW = (frame.right - frame.left);
        const int frameH = (frame.bottom - frame.top);
        const int availW = (mi.rcWork.right - mi.rcWork.left) - frameW;
        const int availH = (mi.rcWork.bottom - mi.rcWork.top) - frameH;

        float scale = 1.0f;

        if (availW < 1920) 
            scale = min(scale, availW / 1920.0f);

        if (availH < 1080) 
            scale = min(scale, availH / 1080.0f);

        const int clientW = static_cast<int>(1920 * scale);
        const int clientH = static_cast<int>(1080 * scale);

        RECT rc = { 0, 0, clientW, clientH };

        AdjustWindowRect(&rc, style, FALSE);

        const int winW = rc.right - rc.left;
        const int winH = rc.bottom - rc.top;

        SetWindowPos(windowHandle, HWND_TOP,
            mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - winW) / 2,  
            mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - winH) / 2,
            winW, winH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
}
