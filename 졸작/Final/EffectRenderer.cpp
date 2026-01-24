#include "pch.h"
#include "EffectRenderer.h"
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

void EffectRenderer::Update(float deltaTime)
{
    if (handle != -1)
    {
        if (auto tr = GetGameObject()->GetComponent<Transform>())
        {
            XMFLOAT3 pos = tr->GetPosition();
            GET(EffectManager).SetLocation(handle, pos);
        }
    }
}

void EffectRenderer::PlayEffect()
{
    if (effectName.empty())
        return;

    auto tr = GetGameObject()->GetComponent<Transform>();
    XMFLOAT3 pos = tr ? tr->GetPosition() : XMFLOAT3(0, 0, 0);

    handle = GET(EffectManager).Play(effectName, pos);
}

void EffectRenderer::StopEffect()
{
    if (handle != -1)
    {
        GET(EffectManager).Stop(handle);
        handle = -1;
    }
}