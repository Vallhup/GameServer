#pragma once
#include "Component.h"
#include <Effekseer.h>

class EffectRenderer : public Component
{
public:
    EffectRenderer();
    virtual ~EffectRenderer();

    void SetEffectName(const std::wstring& name);
    void SetWorldMatrix(const XMMATRIX& mat);

    void Update(float deltaTime) override;

    void PlayEffect();
    void StopEffect();
    bool IsPlaying() const { return handle != -1; }

private:
    std::wstring effectName;
    Effekseer::Handle handle = -1;

    XMMATRIX worldMatrix = XMMatrixIdentity();
};
