#include "pch.h"
#include "DissolveComponent.h"
#include "DX12Core.h"
#include "Material.h"

unsigned int DissolveComponent::s_noiseIndex = 0xFFFFFFFF;
unsigned int DissolveComponent::s_bossNoiseIndex = 0xFFFFFFFF;

void DissolveComponent::RegisterNoiseTexture(DX12Core& core)
{
    s_noiseIndex = Material::RegisterTexture(core.GetDevice(), core.GetGraphicsCmdList(),
        L"../Assets/Effects/Textures/Distortion01.dds");
    s_bossNoiseIndex = Material::RegisterTexture(core.GetDevice(), core.GetGraphicsCmdList(),
        L"../Assets/Effects/Textures/Noise4.dds");
}

void DissolveComponent::Start()
{
    active = true;
    elapsed = 0.0f;
    amount = 0.0f;
}

void DissolveComponent::Reset()
{
    active = false;
    elapsed = 0.0f;
    amount = 0.0f;
}

void DissolveComponent::Update(float deltaTime)
{
    if (!active) return;
    elapsed += deltaTime;
    amount = std::clamp(elapsed / (useBossNoise ? BOSS_DURATION : DURATION), 0.0f, 1.0f);
}
