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
    void Render(const glm::vec3& cameraPos, const glm::vec3& cameraTarget);
    void Release();

    // ¿Ã∆Â∆Æ ¿Áª˝
    int PlayEffect(const std::string& effectName, const glm::vec3& position);
    int PlayEffect(const std::string& effectName, const glm::vec3& position, const glm::vec3& rotation);

    // ¿Ã∆Â∆Æ ¡¶æÓ
    void StopEffect(int handle);
    void StopAllEffects();
    void SetEffectPosition(int handle, const glm::vec3& position);
    void SetEffectRotation(int handle, const glm::vec3& rotation);

    // ¿Ã∆Â∆Æ ∑ŒµÂ
    void LoadEffect(const std::string& name, const char16_t* filePath);

private:
    Effekseer::ManagerRef manager;
    EffekseerRenderer::RendererRef renderer;
    std::map<std::string, Effekseer::EffectRef> effects;

    // GLM to Effekseer ∫Ø»Ø «Ô∆€
    Effekseer::Vector3D ToEffekseerVector(const glm::vec3& vec);
    Effekseer::Matrix44 ToEffekseerMatrix(const glm::mat4& mat);

    float totalTime = 0.0f;
};