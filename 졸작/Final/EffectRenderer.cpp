#include "pch.h"
#include "EffectRenderer.h"
#include "Engine.h"
#include "GameObject.h"
#include "Transform.h"
#include "EffectManager.h"

EffectRenderer::EffectRenderer()
    : handle(-1)
{
}

EffectRenderer::~EffectRenderer()
{
    StopEffect();
}

void EffectRenderer::SetEffectName(const std::wstring& name)
{
    effectName = name;
}

void EffectRenderer::SetWorldMatrix(const XMMATRIX& mat)
{
    worldMatrix = mat;
}

void EffectRenderer::Update(float deltaTime)
{
    if (handle != -1)
    {
        EFFECT_MANAGER->SetMatrix(handle, worldMatrix);
    }
}

void EffectRenderer::PlayEffect()
{
    if (effectName.empty())
        return;

    auto tr = GetGameObject()->GetComponent<Transform>();
    XMFLOAT3 pos = tr ? tr->GetPosition() : XMFLOAT3(0, 0, 0);

    handle = EFFECT_MANAGER->Play(effectName, pos);
}

void EffectRenderer::StopEffect()
{
    if (handle != -1)
    {
        EFFECT_MANAGER->Stop(handle);
        handle = -1;
    }
}