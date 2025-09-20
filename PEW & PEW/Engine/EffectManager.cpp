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
    manager = Effekseer::Manager::Create(8000);

    manager->SetCoordinateSystem(Effekseer::CoordinateSystem::RH);

    renderer = EffekseerRendererGL::Renderer::Create(8000, EffekseerRendererGL::OpenGLDeviceType::OpenGL3);

    manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
    manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
    manager->SetRingRenderer(renderer->CreateRingRenderer());
    manager->SetTrackRenderer(renderer->CreateTrackRenderer());
    manager->SetModelRenderer(renderer->CreateModelRenderer());

    manager->SetTextureLoader(renderer->CreateTextureLoader());
    manager->SetModelLoader(renderer->CreateModelLoader());
    manager->SetMaterialLoader(renderer->CreateMaterialLoader());
    manager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

    LoadEffect("FootSmoke", u"Effects/FootSmoke.efk");
    LoadEffect("ASalamander", u"Effects/ASalamander.efk");
    LoadEffect("Hit", u"Effects/Hit.efk");
    LoadEffect("Hit2", u"Effects/Hit2.efk");
}

void EffectManager::Update(float deltaTime)
{
    if (manager == nullptr)
        return;

    totalTime += deltaTime;
    manager->Update(deltaTime * 60.0f);
}

void EffectManager::Render(const glm::mat4& view, const glm::mat4& projection)
{
    if (renderer == nullptr) return;

    Effekseer::Matrix44 effekseerProjection;
    Effekseer::Matrix44 effekseerView;

    memcpy(&effekseerProjection, glm::value_ptr(projection), sizeof(float) * 16);
    memcpy(&effekseerView, glm::value_ptr(view), sizeof(float) * 16);

    renderer->SetProjectionMatrix(effekseerProjection);
    renderer->SetCameraMatrix(effekseerView);

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
