#include "pch.h"
#include "EffectManager.h"
#include "DX12Core.h"
#include "Camera.h"

void EffectManager::Initialize(DX12Core& core)
{
    manager = Effekseer::Manager::Create(8000);
    manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

    efDevice = EffekseerRendererDX12::CreateGraphicsDevice(core.GetDevice(), core.GetCmdQueue(), SWAP_CHAIN_BUFFER_COUNT);

    DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    renderer = EffekseerRendererDX12::Create(efDevice, &renderTargetFormat, 1, DXGI_FORMAT_D32_FLOAT, false, 8000);
    efMemPool = EffekseerRenderer::CreateSingleFrameMemoryPool(efDevice);
    efCmdList = EffekseerRenderer::CreateCommandList(efDevice, efMemPool);

    manager->SetModelRenderer(renderer->CreateModelRenderer());
    manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
    manager->SetRingRenderer(renderer->CreateRingRenderer());
    manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
    manager->SetTrackRenderer(renderer->CreateTrackRenderer());

    manager->SetTextureLoader(renderer->CreateTextureLoader());
    manager->SetModelLoader(renderer->CreateModelLoader());
    manager->SetMaterialLoader(renderer->CreateMaterialLoader());
    manager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

    OutputDebugStringA("EffectManager initialized!\n");
}

void EffectManager::Update(float deltaTime)
{
    totalTime += deltaTime;
    manager->Update(deltaTime * 60.0f);
}

void EffectManager::Render(DX12Core& core, Camera* camera)
{
    if (!camera) return;

    XMFLOAT3 camPos = camera->GetPosition();
    XMFLOAT3 camTarget = camera->GetTargetPosition();

    Effekseer::Matrix44 projectionMatrix;
    float aspectRatio = static_cast<float>(WinSize.x) / static_cast<float>(WinSize.y);
    projectionMatrix.PerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);

    Effekseer::Matrix44 cameraMatrix;
    cameraMatrix.LookAtLH(
        Effekseer::Vector3D(camPos.x, camPos.y, camPos.z),
        Effekseer::Vector3D(camTarget.x, camTarget.y, camTarget.z),
        Effekseer::Vector3D(0.0f, 1.0f, 0.0f)
    );

    Effekseer::Manager::LayerParameter layerParameter;
    layerParameter.ViewerPosition = Effekseer::Vector3D(camPos.x, camPos.y, camPos.z);
    manager->SetLayerParameter(0, layerParameter);

    renderer->SetTime(totalTime);
    renderer->SetProjectionMatrix(projectionMatrix);
    renderer->SetCameraMatrix(cameraMatrix);

    efMemPool->NewFrame();

    auto dx12Cmd = core.GetGraphicsCmdList();
    EffekseerRendererDX12::BeginCommandList(efCmdList, dx12Cmd);
    renderer->SetCommandList(efCmdList);

    renderer->BeginRendering();
    manager->Draw();
    renderer->EndRendering();

    EffekseerRendererDX12::EndCommandList(efCmdList);
}

void EffectManager::Release()
{
    effectCache.clear();

    if (manager != nullptr) manager.Reset();
    if (renderer != nullptr) renderer.Reset();
    if (efCmdList != nullptr) efCmdList.Reset();
    if (efMemPool != nullptr) efMemPool.Reset();
    if (efDevice != nullptr) efDevice.Reset();

    OutputDebugStringA("EffectManager Released!\n");
}

void EffectManager::PreLoad(const wstring& effectName)
{
    if (effectCache.find(effectName) != effectCache.end())
        return;

    wstring path = L"../Assets/Effects/" + effectName + L".efk";
    auto effect = Effekseer::Effect::Create(manager, (const char16_t*)path.c_str());

    if (effect == nullptr) {
        OutputDebugStringA("Failed to load effect!\n");
        return;
    }
    effectCache[effectName] = effect;

    OutputDebugStringW((L"Effect Preloaded: " + effectName + L"\n").c_str());
}

Effekseer::Handle EffectManager::Play(const wstring& effectName, const XMFLOAT3& pos)
{
    if (effectCache.find(effectName) == effectCache.end())
    {
        wstring msg = L"[Warning] Effect NOT Preloaded: " + effectName + L"\n";
        OutputDebugStringW(msg.c_str());
        return -1;
    }
 
    auto handle = manager->Play(effectCache[effectName], pos.x, pos.y, pos.z);

    return handle;
}

void EffectManager::SetLocation(Effekseer::Handle handle, const XMFLOAT3& pos)
{
    manager->SetLocation(handle, pos.x, pos.y, pos.z);
}

void EffectManager::SetMatrix(Effekseer::Handle handle, const XMMATRIX& mat)
{
    Effekseer::Matrix43 efMat;

    efMat.Value[0][0] = mat.r[0].m128_f32[0];
    efMat.Value[0][1] = mat.r[0].m128_f32[1];
    efMat.Value[0][2] = mat.r[0].m128_f32[2];
    efMat.Value[1][0] = mat.r[1].m128_f32[0];
    efMat.Value[1][1] = mat.r[1].m128_f32[1];
    efMat.Value[1][2] = mat.r[1].m128_f32[2];
    efMat.Value[2][0] = mat.r[2].m128_f32[0];
    efMat.Value[2][1] = mat.r[2].m128_f32[1];
    efMat.Value[2][2] = mat.r[2].m128_f32[2];
    efMat.Value[3][0] = mat.r[3].m128_f32[0];  // position x
    efMat.Value[3][1] = mat.r[3].m128_f32[1];  // position y
    efMat.Value[3][2] = mat.r[3].m128_f32[2];  // position z

    manager->SetMatrix(handle, efMat);
}

void EffectManager::Stop(Effekseer::Handle handle)
{
    if (manager == nullptr)
        return;

    manager->StopEffect(handle);
}
