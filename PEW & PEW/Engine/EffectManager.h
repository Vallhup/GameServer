#pragma once
#include <Effekseer.h>
#include <EffekseerRendererGL.h>

class EffectManager
{
public:
    EffectManager();
    ~EffectManager();

    void Init();
    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void Release();

    int PlayEffect(const std::string& effectName, const glm::vec3& position);
    int PlayEffect(const std::string& effectName, const glm::vec3& position, const glm::vec3& rotation);

    void StopEffect(int handle);
    void StopAllEffects();
    void SetEffectPosition(int handle, const glm::vec3& position);
    void SetEffectRotation(int handle, const glm::vec3& rotation);

    void LoadEffect(const std::string& name, const char16_t* filePath);
    Effekseer::Vector3D ToEffekseerVector(const glm::vec3& vec);

private:
    Effekseer::ManagerRef manager;
    EffekseerRenderer::RendererRef renderer;
    std::map<std::string, Effekseer::EffectRef> effects;

    float totalTime = 0.0f;
};