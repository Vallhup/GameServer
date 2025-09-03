#pragma once
#include "Component.h"
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

class DX12Core;

class EffectRenderer : public Component
{
public:
    EffectRenderer();
    ~EffectRenderer();

    void Initialize(DX12Core& core);
    void Update(float deltaTime) override;
    void Render(DX12Core& core);

    void PlayEffect();

private:
    Effekseer::ManagerRef manager;
    EffekseerRenderer::RendererRef renderer;
    Effekseer::EffectRef effect;
    Effekseer::Handle handle;
};

