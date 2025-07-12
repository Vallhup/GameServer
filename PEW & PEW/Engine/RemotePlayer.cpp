#include "pch.h"
#include "RemotePlayer.h"
#include "ShadowMapping.h"

RemotePlayer::RemotePlayer(int id) : playerID(id)
{
    player_BoneInfo = new vector<BoneInfo>();
    animModel = new AnimatedModel();
    player_CurrentAnim = new AnimInfo();
    animLibrary = new AnimatedModel::AnimationLibrary();

    characterPos = glm::vec3(0.0f, 0.0f, 0.0f);
    targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
    currentDirection = -1;
    isRunning = false;
}

RemotePlayer::~RemotePlayer()
{
    delete player_CurrentAnim;
    delete player_BoneInfo;
    delete animModel;

    if (animLibrary != nullptr) {
        delete animLibrary;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &Texture);
    glDeleteProgram(shaderprogram);
}

void RemotePlayer::Init()
{
    SaveAnimations();

    animModel->LoadGLBFile(0, *player_BoneInfo, "Glb/cat_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
    Texture = LoadTexture("Texture/CatTexture.png");
    SetupShader("Shaders/CatVert.glsl", "Shaders/CatFrag.glsl", shaderprogram);
}

void RemotePlayer::Update(float deltaTime)
{
    // 부드러운 위치 보간
    glm::vec3 direction = targetPos - characterPos;
    float distance = glm::length(direction);

    if (distance > 0.001f) {
        characterPos = glm::mix(characterPos, targetPos, lerpSpeed * deltaTime);
    }

    UpdateAnimation();
}

void RemotePlayer::Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime)
{
    animModel->UpdateAnimation(0, *player_BoneInfo, deltaTime, *player_CurrentAnim);
    glUseProgram(shaderprogram);
    animModel->SetupBoneTransforms(*player_BoneInfo, shaderprogram);

    model = glm::mat4(1.0f);
    model = glm::translate(model, characterPos);

    // 방향에 따른 회전 각도 계산 (필요시)
    float angle = 0.0f;
    if (currentDirection != -1) {
        // direction에 따른 각도 계산 로직 추가 가능
    }
    // model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    ViewLoc = glGetUniformLocation(shaderprogram, "view");
    glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &view[0][0]);
    ProjLoc = glGetUniformLocation(shaderprogram, "projection");
    glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &projection[0][0]);
    ModelLoc = glGetUniformLocation(shaderprogram, "model");
    glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, &model[0][0]);

    // 조명 설정 (MainCharacter와 동일)
    GLuint lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderprogram, "viewPos");
    glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    TextureLoc = glGetUniformLocation(shaderprogram, "catTexture");
    glUniform1i(TextureLoc, 0);

    GLuint colorHitLoc = glGetUniformLocation(shaderprogram, "colorHit");
    glUniform4fv(colorHitLoc, 1, glm::value_ptr(hitcolor));

    UseTextureLoc = glGetUniformLocation(shaderprogram, "useTexture");
    glUniform1i(UseTextureLoc, 1);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void RemotePlayer::DrawShadow(GLuint depthShaderProgram, const glm::mat4& lightSpaceMatrix)
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, characterPos);
    // model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    ModelLoc = glGetUniformLocation(depthShaderProgram, "model");
    glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    animModel->SetupBoneTransforms(*player_BoneInfo, depthShaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
}

void RemotePlayer::UpdateFromPacket(float x, float y, float z, char direction, bool run)
{
    SetTargetPosition(x, y, z);
    currentDirection = direction;
    isRunning = run;
}

void RemotePlayer::SetTargetPosition(float x, float y, float z)
{
    targetPos = glm::vec3(x, y, z);
}

void RemotePlayer::SaveAnimations()
{
    // MainCharacter와 동일한 애니메이션들 로드
    animLibrary->LoadAnimation("Idle", "Animations/cat_animation_idle.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Die", "Animations/cat_animation_die.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Walk", "Animations/cat_animation_walking.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Run", "Animations/cat_animation_run.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Hit", "Animations/cat_animation_hit.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Jump", "Animations/cat_animation_jump.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("AimIdle", "Animations/cat_animation_idle_aim.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("AimWalk", "Animations/cat_animation_walking_aim.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Fire", "Animations/cat_animation_firing_aim.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("FireWalk", "Animations/cat_animation_firing_walk.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("FireRun", "Animations/cat_animation_firing_run.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Dance", "Animations/cat_animation_dance.glb", animationImporters, animModel);

    animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
}

void RemotePlayer::UpdateAnimation()
{
    // 움직임과 속도에 따른 애니메이션 변경
    glm::vec3 velocity = targetPos - characterPos;
    float speed = glm::length(velocity);

    if (speed > 0.001f) {
        if (isRunning) {
            if (animLibrary->GetCurrentAnimation() != "Run") {
                animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
            }
        }
        else {
            if (animLibrary->GetCurrentAnimation() != "Walk") {
                animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
            }
        }
    }
    else {
        if (animLibrary->GetCurrentAnimation() != "Idle") {
            animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
        }
    }
}
