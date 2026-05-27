#pragma once
#include "Component.h"

class DX12Core;

class DissolveComponent : public Component
{
public:
    static constexpr float DURATION = 1.8f;        
    static constexpr float BOSS_DURATION = 3.0f;   

    static void RegisterNoiseTexture(DX12Core& core);

    void Update(float deltaTime) override;

    void Start();
    void Reset();

    bool IsActive() const { return active; }
    bool IsFinished() const { return active && amount >= 1.0f; }
    float GetAmount() const { return amount; }
    unsigned int GetNoiseIndex() const { return useBossNoise ? s_bossNoiseIndex : s_noiseIndex; }

    void UseBossNoise(bool v) { useBossNoise = v; }

private:
    static unsigned int s_noiseIndex;
    static unsigned int s_bossNoiseIndex;

    bool active = false;
    bool useBossNoise = false;
    float elapsed = 0.0f;
    float amount = 0.0f;
};
