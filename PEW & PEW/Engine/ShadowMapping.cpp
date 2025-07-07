#include "pch.h"
#include "ShadowMapping.h"

ShadowMapping::ShadowMapping() : depthMapFBO(0), depthMap(0), depthMapFBO_enemy(0), depthMap_enemy(0) {
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glGenFramebuffers(1, &depthMapFBO_enemy);

    glGenTextures(1, &depthMap_enemy);
    glBindTexture(GL_TEXTURE_2D, depthMap_enemy);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_enemy);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap_enemy, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    InitShaders();
}

ShadowMapping::~ShadowMapping() {
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
    glDeleteFramebuffers(1, &depthMapFBO_enemy);
    glDeleteTextures(1, &depthMap_enemy);
}

void ShadowMapping::BindFramebuffer() const {
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
}

void ShadowMapping::UnbindFramebuffer() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapping::UpdateLightSpaceMatrix(const glm::vec3& currentCharacterPos) {
    float near_plane = 0.1f;
    float far_plane = 100.0f;
    float ortho_size = 12.0f;

    glm::vec3 adjustedLightPos = currentCharacterPos + glm::vec3(-(60.0f * cos(light_angle)), 60.0f, 0.0f + (60.0f * sin(light_angle)));
    glm::vec3 lightTarget = currentCharacterPos;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    lightProjection = glm::ortho(-ortho_size, ortho_size,
        -ortho_size, ortho_size,
        near_plane, far_plane);

    lightView = glm::lookAt(adjustedLightPos, lightTarget, up);
    lightSpaceMatrix = lightProjection * lightView;
}

void ShadowMapping::InitShaders() {
    SetupShader("Shaders/ShadowMappingVert.glsl", "Shaders/ShadowMappingFrag.glsl", depthShaderProgram);
    SetupShader("Shaders/ShadowStaticMappingVert.glsl", "Shaders/ShadowStaticMappingFrag.glsl", staticdepthShaderProgram);
}