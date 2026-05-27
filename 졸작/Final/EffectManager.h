#pragma once
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

class DX12Core;
class Camera;

class EffectManager
{
public:
    void Initialize(DX12Core& core);
    void Update(float deltaTime);
    void Render(DX12Core& core, Camera* camera);
    void Release();

    void PreLoad(const wstring& effectName);

    Effekseer::Handle Play(const wstring& effectName, const XMFLOAT3& pos);
    void SetLocation(Effekseer::Handle handle, const XMFLOAT3& pos);
    void SetMatrix(Effekseer::Handle handle, const XMMATRIX& mat);
    void Stop(Effekseer::Handle handle);
    bool Exists(Effekseer::Handle handle) const;

private:
    float totalTime = 0.0f;

    Effekseer::ManagerRef manager;
    EffekseerRenderer::RendererRef renderer;
    Effekseer::Backend::GraphicsDeviceRef efDevice;
    Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> efMemPool;
    Effekseer::RefPtr<EffekseerRenderer::CommandList> efCmdList;

    map<wstring, Effekseer::EffectRef> effectCache;
};
