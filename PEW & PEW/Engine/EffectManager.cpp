#include "pch.h"
#include "EffectManager.h"

EffectManager::EffectManager()
{
    manager = nullptr;
    renderer = nullptr;
}

EffectManager::~EffectManager()
{
    Release();
}

void EffectManager::Init()
{
    // 매니저 생성
    manager = Effekseer::Manager::Create(8000);

    manager->SetCoordinateSystem(Effekseer::CoordinateSystem::RH);

    // 렌더러 생성
    renderer = EffekseerRendererGL::Renderer::Create(8000, EffekseerRendererGL::OpenGLDeviceType::OpenGL3);

    // 렌더러를 매니저에 설정
    manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
    manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
    manager->SetRingRenderer(renderer->CreateRingRenderer());
    manager->SetTrackRenderer(renderer->CreateTrackRenderer());
    manager->SetModelRenderer(renderer->CreateModelRenderer());

    manager->SetTextureLoader(renderer->CreateTextureLoader());
    manager->SetModelLoader(renderer->CreateModelLoader());
    manager->SetMaterialLoader(renderer->CreateMaterialLoader());
    manager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

    LoadEffect("CandleFire", u"Effects/CandleFire3.efk");
}

void EffectManager::Update(float deltaTime)
{
    if (manager == nullptr)
        return;

    totalTime += deltaTime;
    manager->Update(deltaTime * 60.0f);
}

void EffectManager::Render(const glm::vec3& cameraPos, const glm::vec3& cameraTarget)
{
    if (renderer == nullptr)
        return;

    // Effekseer 내장 함수 사용 (DX12 방식)
    auto viewerPosition = Effekseer::Vector3D(cameraPos.x, cameraPos.y, cameraPos.z);
    auto targetPosition = Effekseer::Vector3D(cameraTarget.x, cameraTarget.y, cameraTarget.z);

    // 투영 행렬 생성
    Effekseer::Matrix44 projectionMatrix;
    float aspectRatio = (float)WIN_W / (float)WIN_H;
    projectionMatrix.PerspectiveFovRH(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

    // 카메라 행렬 생성
    Effekseer::Matrix44 cameraMatrix;
    cameraMatrix.LookAtRH(viewerPosition, targetPosition, Effekseer::Vector3D(0.0f, 1.0f, 0.0f));

    Effekseer::Manager::LayerParameter layerParam;
    layerParam.ViewerPosition = viewerPosition;
    manager->SetLayerParameter(0, layerParam);

    // 렌더러 설정
    renderer->SetProjectionMatrix(projectionMatrix);
    renderer->SetCameraMatrix(cameraMatrix);

    // 렌더링
    renderer->BeginRendering();
    manager->Draw();
    renderer->EndRendering();
}

void EffectManager::Release()
{
    if (manager != nullptr)
    {
        manager->StopAllEffects();
        manager.Reset();
    }

    if (renderer != nullptr)
    {
        renderer.Reset();
    }

    effects.clear();
}

int EffectManager::PlayEffect(const std::string& effectName, const glm::vec3& position)
{
    auto it = effects.find(effectName);
    if (it != effects.end() && manager != nullptr)
    {
        return manager->Play(it->second, ToEffekseerVector(position));
    }
    return -1;
}

int EffectManager::PlayEffect(const std::string& effectName, const glm::vec3& position, const glm::vec3& rotation)
{
    auto it = effects.find(effectName);
    if (it != effects.end() && manager != nullptr)
    {
        int handle = manager->Play(it->second, ToEffekseerVector(position));
        if (handle != -1)
        {
            SetEffectRotation(handle, rotation);
        }
        return handle;
    }
    return -1;
}

void EffectManager::StopEffect(int handle)
{
    if (manager != nullptr && handle != -1)
    {
        manager->StopEffect(handle);
    }
}

void EffectManager::StopAllEffects()
{
    if (manager != nullptr)
    {
        manager->StopAllEffects();
    }
}

void EffectManager::SetEffectPosition(int handle, const glm::vec3& position)
{
    if (manager != nullptr && handle != -1)
    {
        manager->SetLocation(handle, ToEffekseerVector(position));
    }
}

void EffectManager::SetEffectRotation(int handle, const glm::vec3& rotation)
{
    if (manager != nullptr && handle != -1)
    {
        manager->SetRotation(handle, rotation.x, rotation.y, rotation.z);
    }
}

void EffectManager::LoadEffect(const std::string& name, const char16_t* filePath)
{
    if (manager != nullptr)
    {
        auto effect = Effekseer::Effect::Create(manager, filePath);

        if (effect != nullptr)
        {
            effects[name] = effect;
            std::cout << "Effect loaded: " << name << std::endl;
        }
        else
        {
            std::cout << "Failed to load effect" << std::endl;
        }
    }
}

Effekseer::Vector3D EffectManager::ToEffekseerVector(const glm::vec3& vec)
{
    return Effekseer::Vector3D(vec.x, vec.y, vec.z);
}

Effekseer::Matrix44 EffectManager::ToEffekseerMatrix(const glm::mat4& mat)
{
    Effekseer::Matrix44 result;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.Values[i][j] = mat[j][i]; // GLM은 column-major, Effekseer는 row-major
        }
    }
    return result;
}