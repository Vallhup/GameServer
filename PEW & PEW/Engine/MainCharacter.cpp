#include "pch.h"
#include "MainCharacter.h"
#include "Bullet.h"
#include "ShadowMapping.h"
#include "Camera.h"
#include "BoundingBox.h"

MainCharacter::MainCharacter(int id, bool isLocal) : playerID(id), isLocalPlayer(isLocal)
{
    player_BoneInfo = new vector<BoneInfo>();
    animModel = new AnimatedModel();
    player_CurrentAnim = new AnimInfo();
    animLibrary = new AnimatedModel::AnimationLibrary();

    if (isLocalPlayer)
        hitbox = new BoundingBox();

    characterPos = glm::vec3(-37.3051f, 0.0f, 42.5001f);
    targetPos = characterPos;

    for (int i = 0; i < MAX_BULLETS; ++i)
        bullets[i].bullet = new Bullet();
}

MainCharacter::~MainCharacter()
{
    if (isLocalPlayer) {
        delete hitbox;
    }
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

    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        delete bullets[i].bullet;
        bullets[i].bullet = nullptr;
    }
}

void MainCharacter::Init()
{
    SaveAnimations();

    animModel->LoadGLBFile(0, *player_BoneInfo, "Glb/cat_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
    Texture = LoadTexture("Texture/CatTexture.png");
    SetupShader("Shaders/CatVert.glsl", "Shaders/CatFrag.glsl", shaderprogram);
}

void MainCharacter::Update(float deltaTime)
{
    UpdateAllPlayersMovement(deltaTime);

    UpdateAnimation();
    UpdateHitDecision();
    UpdateBullets(deltaTime);
}

void MainCharacter::Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime, glm::mat4 lightSpaceMatrix, GLuint depthMap)
{
    if (dead)
        return;

    // 로컬 플레이어만 히트박스 렌더링
    if (isLocalPlayer && GetHitBox())
        hitbox->RenderHitbox(angle, characterPos, view, projection);

    animModel->UpdateAnimation(0, *player_BoneInfo, deltaTime, *player_CurrentAnim);
    glUseProgram(shaderprogram);
    animModel->SetupBoneTransforms(*player_BoneInfo, shaderprogram);

    model = glm::mat4(1.0f);
    model = glm::translate(model, characterPos);
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    ViewLoc = glGetUniformLocation(shaderprogram, "view");
    glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &view[0][0]);
    ProjLoc = glGetUniformLocation(shaderprogram, "projection");
    glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &projection[0][0]);
    ModelLoc = glGetUniformLocation(shaderprogram, "model");
    glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, &model[0][0]);

    GLuint lightSpaceMatrixLoc = glGetUniformLocation(shaderprogram, "lightSpaceMatrix");
    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    GLuint shadowMapLoc = glGetUniformLocation(shaderprogram, "shadowMap");
    glUniform1i(shadowMapLoc, 1);

    GLuint lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderprogram, "viewPos");
    glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    TextureLoc = glGetUniformLocation(shaderprogram, "catTexture");
    glUniform1i(TextureLoc, 0);

    colorHitLoc = glGetUniformLocation(shaderprogram, "colorHit");
    glUniform4fv(colorHitLoc, 1, glm::value_ptr(hitcolor));

    UseTextureLoc = glGetUniformLocation(shaderprogram, "useTexture");
    glUniform1i(UseTextureLoc, 1);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void MainCharacter::DrawShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShaderProgram)
{
    if (dead)
        return;

    model = glm::mat4(1.0f);
    model = glm::translate(model, characterPos);
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(depthShaderProgram);			// Depth map 렌더링
    GLuint lightSpaceMatrixLoc = glGetUniformLocation(depthShaderProgram, "lightSpaceMatrix");
    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    ModelLoc = glGetUniformLocation(depthShaderProgram, "model");
    glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    animModel->SetupBoneTransforms(*player_BoneInfo, depthShaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
}

void MainCharacter::RenderBullets(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
    // 모든 캐릭터의 총알 렌더링 (로컬/원격 구분 없이)
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].isActive && bullets[i].bullet) {
            bullets[i].bullet->Render(orgview, orgproj, viewPos, lightSpaceMatrix, shadowMap);
        }
    }
}

void MainCharacter::RenderBulletsShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
    // 모든 캐릭터의 총알 그림자 렌더링 (로컬/원격 구분 없이)
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].isActive && bullets[i].bullet) {
            bullets[i].bullet->RenderShadow(lightSpaceMatrix, depthShader);
        }
    }
}

void MainCharacter::CreateBulletFromServer(int bulletID, glm::vec3 startPos)
{
    // 빈 슬롯 찾기
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].isActive) {
            bullets[i].bulletID = bulletID;
            bullets[i].isActive = true;

            // 서버에서 받은 위치와 방향으로 설정
            bullets[i].bullet->SetPosition(startPos);

            /*std::cout << "[CREATE BULLET FROM SERVER] Player: " << playerID
                << ", Bullet ID: " << bulletID << ", Slot: " << i << std::endl;*/
            return;
        }
    }
}

bool MainCharacter::RemoveBulletFromServer(int bulletID)
{
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].isActive && bullets[i].bulletID == bulletID) {
            bullets[i].bulletID = -1;
            bullets[i].isActive = false;

            /*std::cout << "[REMOVE BULLET FROM SERVER] Player: " << playerID
                << ", Bullet ID: " << bulletID << ", Slot: " << i << std::endl;*/
            return true;
        }
    }

    return false;
}

void MainCharacter::UpdateBullets(float deltaTime)
{
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (bullets[i].isActive && bullets[i].bullet) {
            bullets[i].bullet->BulletUpdate(deltaTime);
        }
    }
}

bool MainCharacter::UpdateBulletFromServer(int bulletID, glm::vec3 newPos)
{
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].isActive && bullets[i].bulletID == bulletID) {
            bullets[i].bullet->SetPosition(newPos);
            return true;
        }
    }

    return false;
}

void MainCharacter::UpdateAllPlayersMovement(float deltaTime)
{
    glm::vec3 direction = targetPos - characterPos;
    float distance = glm::length(direction);

    if (distance > 0.001f) {
        characterPos = glm::mix(characterPos, targetPos, lerpSpeed * deltaTime);
    }
}

void MainCharacter::UpdateHitDecision()
{
    if (hit_cnt > 0)
        hit_cnt--;
    else
    {
        if (hitcolor != glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
            hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void MainCharacter::SaveAnimations()
{
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

void MainCharacter::UpdateAnimation()
{
    if (dying)
    {
        if (animLibrary->GetCurrentAnimation() != "Die")
            animLibrary->ChangeAnimation("Die", *player_CurrentAnim);
        else
        {
            if (player_CurrentAnim->CurrentTime + 10 >= player_CurrentAnim->Duration)
                SetDead(true);
        }

        return;
    }

    glm::vec3 velocity = targetPos - characterPos;
    float speed = glm::length(velocity);

    std::string currentAnim = GetAnimLibrary()->GetCurrentAnimation();
    bool isFireAnim = (currentAnim == "Fire" || currentAnim == "FireWalk" || currentAnim == "FireRun");

    if (firing)
    {
        if (isMoving)
        {
            if (isRunning)
            {
                if (!isFireAnim) {
                    animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                }
            }
            else
            {
                if (!isFireAnim) {
                    animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                }
            }
        }
        else
        {
            if (!isFireAnim) {
                animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
            }
        }
    }
    else
    {
        if (!isFireAnim)
        {
            if (isMoving)
            {
                if (isRunning)
                {
                    if (animLibrary->GetCurrentAnimation() != "Run") {
                        animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                    }
                }
                else
                {
                    if (animLibrary->GetCurrentAnimation() != "Walk") {
                        animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                    }
                }
            }
            else
            {
                if (animLibrary->GetCurrentAnimation() != "Idle") {
                    animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
                }
            }
        }
    }

    if (animLibrary->GetCurrentAnimation() == "FireRun")
    {
        if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
        {
            if (!firing)
            {
                if (isMoving)
                {
                    if (isRunning)
                        animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                    else
                        animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                }
                else
                    animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
            }
            else
            {
                if (isMoving)
                {
                    if (!isRunning)
                        animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                    else
                        animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                }
                else
                    animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
            }
        }
    }
    else if (animLibrary->GetCurrentAnimation() == "FireWalk")
    {
        if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
        {
            if (!firing)
            {
                if (isMoving)
                {
                    if (isRunning)
                        animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                    else
                        animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                }
                else
                    animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
            }
            else
            {
                if (isMoving)
                {
                    if (isRunning)
                        animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                    else
                        animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                }
                else
                    animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
            }
        }
    }
    else if (animLibrary->GetCurrentAnimation() == "Fire")
    {
        if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
        {
            if (!firing)
            {
                if (isMoving)
                {
                    if (isRunning)
                        animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                    else
                        animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                }
                else
                    animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
            }
            else
            {
                if (isMoving)
                {
                    if (isRunning)
                        animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                    else
                        animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                }
                else
                    animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
            }
        }
    }
    else if (animLibrary->GetCurrentAnimation() == "Die")
        animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
}

void MainCharacter::UpdateFromPacket(float ang, float x, float y, float z, char direction, bool move, bool run)
{
    if (!isLocalPlayer)
        angle = ang;
    SetTargetPosition(x, y, z);
    currentDirection = direction;
    isMoving = move;
    isRunning = run;
}

void MainCharacter::ReviveFromPacket(float x, float y, float z)
{
    characterPos = glm::vec3(x, y, z);
    SetTargetPosition(x, y, z);
    dead = false;
}

void MainCharacter::DamagedFromPacket()
{
    hit_cnt = 200;
    hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
}

void MainCharacter::SetTargetPosition(float x, float y, float z)
{
    targetPos = glm::vec3(x, y, z);
}