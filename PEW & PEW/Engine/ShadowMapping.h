#pragma once

class ShadowMapping {
public:
    ShadowMapping();
    ~ShadowMapping();

    void BindFramebuffer() const;
    void UnbindFramebuffer() const;

    void UpdateLightSpaceMatrix(const glm::vec3& currentCharacterPos);

    unsigned int GetDepthMapFBO() const { return depthMapFBO; }
    unsigned int GetDepthMap() const { return depthMap; }

    unsigned int GetDepthMapFBO_Enemy() const { return depthMapFBO_enemy; }
    unsigned int GetDepthMap_Enemy() const { return depthMap_enemy; }

    glm::mat4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }
    glm::mat4 GetLightSpaceMatrix_Enemy() const { return lightSpaceMatrix_enemy; }

    GLuint GetDepthShaderProgram() const { return depthShaderProgram; }
    GLuint GetStaticDepthShaderProgram() const { return staticdepthShaderProgram; }

private:
    const unsigned int SHADOW_WIDTH = { 4096 };
    const unsigned int SHADOW_HEIGHT = { 4096 };

    unsigned int depthMapFBO;
    unsigned int depthMap;

    unsigned int depthMapFBO_enemy;
    unsigned int depthMap_enemy;

    glm::mat4 lightProjection, lightProjection_enemy;
    glm::mat4 lightView, lightView_enemy;
    glm::mat4 lightSpaceMatrix, lightSpaceMatrix_enemy;
    GLuint depthShaderProgram;
    GLuint staticdepthShaderProgram;

    void InitShaders();
};