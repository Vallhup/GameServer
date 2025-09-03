#include "pch.h"
#include "EffectRenderer.h"
#include "DX12Core.h"

EffectRenderer::EffectRenderer()
    : handle(-1)
{
}

EffectRenderer::~EffectRenderer()
{
    if (handle != -1 && manager != nullptr)
    {
        manager->StopEffect(handle);
    }
}

void EffectRenderer::Initialize(DX12Core& core)
{
    // Effekseer 초기화
    manager = Effekseer::Manager::Create(8000);

    DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    renderer = EffekseerRendererDX12::Create(
        core.GetDevice(),           // device
        core.GetCmdQueue(),         // commandQueue  
        2,                          // swapBufferCount (SWAP_CHAIN_BUFFER_COUNT와 일치)
        &renderTargetFormat,        // renderTargetFormats (Forward Pass - 백버퍼)
        1,                          // renderTargetCount (Forward는 1개)
        DXGI_FORMAT_D32_FLOAT,      // depthFormat 
        false,                      // isReversedDepth
        8000                        // squareMaxCount
    );

    manager->SetModelRenderer(renderer->CreateModelRenderer());
    manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
    manager->SetRingRenderer(renderer->CreateRingRenderer());
    manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
    manager->SetTrackRenderer(renderer->CreateTrackRenderer());

    OutputDebugStringA("EffectRenderer initialized!\n");
}

void EffectRenderer::Update(float deltaTime)
{
    if (manager != nullptr)
    {
        manager->Update(deltaTime * 60.0f); // Effekseer는 60fps 기준
    }
}

void EffectRenderer::Render(DX12Core& core)
{
    if (renderer != nullptr)
    {
        renderer->BeginRendering();
        manager->Draw();
        renderer->EndRendering();
    }
}

void EffectRenderer::PlayEffect()
{
    // 테스트용 - 나중에 .efk 파일 로딩으로 교체
    OutputDebugStringA("PlayEffect called!\n");
}