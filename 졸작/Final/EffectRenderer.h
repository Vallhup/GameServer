#pragma once
#include "Component.h"
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

class DX12Core;
class Camera;

class EffectRenderer : public Component
{
public:
    EffectRenderer();
    ~EffectRenderer();

    void Initialize(DX12Core& core);
    void Update(float deltaTime) override;
    void Render(DX12Core& core, Camera* camera);

    void LoadEffect(const char16_t* effectPath);
    void PlayEffect();
    void StopEffect();

private:
    Effekseer::ManagerRef manager;
    EffekseerRenderer::RendererRef renderer;
    Effekseer::EffectRef effect;
    Effekseer::Handle handle;
    float totalTime = 0.0f;

    ::Effekseer::Backend::GraphicsDeviceRef efDevice;
    Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> efMemPool;
    Effekseer::RefPtr<EffekseerRenderer::CommandList> efCmdList;
};

