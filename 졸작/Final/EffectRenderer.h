#pragma once
#include "Component.h"
#include <Effekseer.h>

class EffectRenderer : public Component
{
public:
    EffectRenderer();
    virtual ~EffectRenderer();

    void SetEffectName(const wstring& name);
    void SetWorldMatrix(const XMMATRIX& mat);

    void Update(float deltaTime) override;

    void PlayEffect();
    void StopEffect();
    bool IsPlaying() const { return handle != -1; }

private:
    wstring effectName;
    Effekseer::Handle handle = -1;
};
