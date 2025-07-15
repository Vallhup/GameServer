#include "pch.h"
#include "Character.h"
#include "ShadowMapping.h"
#include "Bullet.h"
#include "Camera.h"
#include "BoundingBox.h"

Character::Character(int id, bool isLocal) : playerID(id), isLocalPlayer(isLocal)
{
    player_BoneInfo = new vector<BoneInfo>();
    animModel = new AnimatedModel();
    player_CurrentAnim = new AnimInfo();
    animLibrary = new AnimatedModel::AnimationLibrary();

    if (isLocalPlayer) 
        hitbox = new BoundingBox();
    else 
        hitbox = nullptr;

    characterPos = glm::vec3(-37.3051f, 0.0f, 42.5001f);
    targetPos = characterPos;
}

Character::~Character()
{
    if (isLocalPlayer && hitbox) {
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
}

void Character::Init()
{
    SaveAnimations();

    animModel->LoadGLBFile(0, *player_BoneInfo, "Glb/cat_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
    Texture = LoadTexture("Texture/CatTexture.png");
    SetupShader("Shaders/CatVert.glsl", "Shaders/CatFrag.glsl", shaderprogram);
}

void Character::Update(float deltaTime)
{
    /*if (isLocalPlayer) {
        HandleLocalPlayerUpdate(deltaTime);
    }
    else {*/
        HandleRemotePlayerUpdate(deltaTime);
    /*}*/

    UpdateAnimation();
}

void Character::Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime, float angle)
{
    // 로컬 플레이어만 히트박스 렌더링
    if (isLocalPlayer && hitbox_ison())
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

void Character::DrawShadow(float angle, GLuint depthShaderProgram, const glm::mat4& lightSpaceMatrix)
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, characterPos);
    if (!dying)
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    else
        model = glm::rotate(model, lastangle, glm::vec3(0.0f, 1.0f, 0.0f));

    ModelLoc = glGetUniformLocation(depthShaderProgram, "model");
    glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    animModel->SetupBoneTransforms(*player_BoneInfo, depthShaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
}

void Character::DrawBulletShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    for (auto& bullet : bullets)
    {
        bullet->RenderShadow(lightSpaceMatrix, depthShader);
    }
}

void Character::HandleLocalPlayerUpdate(float deltaTime)
{
    // MainCharacter의 기존 Update 로직
    if (IsMoving()) {
        if (Shift_value())
            Run(deltaTime);
        else
            Walk(deltaTime);
    }
}

void Character::HandleRemotePlayerUpdate(float deltaTime)
{
    // RemotePlayer의 기존 Update 로직 (부드러운 보간)
    glm::vec3 direction = targetPos - characterPos;
    float distance = glm::length(direction);

    if (distance > 0.001f) {
        characterPos = glm::mix(characterPos, targetPos, lerpSpeed * deltaTime);
    }
}

void Character::Walk()
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    if (!camera->GetViewType()) {
        if (_Right)
            characterPos.x += 0.005f;
        if (_Left)
            characterPos.x -= 0.005f;
        if (_Top)
            characterPos.z -= 0.005f;
        if (_Bottom)
            characterPos.z += 0.005f;
    }
    else {
        glm::vec3 forward(
            sin(camera->GetHorizontalAngle()),
            0,
            cos(camera->GetHorizontalAngle())
        );
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

        glm::vec3 moveDir(0.0f);
        if (_Top) moveDir += forward;
        if (_Bottom) moveDir -= forward;
        if (_Right) moveDir += right;
        if (_Left) moveDir -= right;

        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 nextPos = characterPos + moveDir * 0.005f;
            characterPos = nextPos;
        }
    }
}

void Character::Walk(float deltaTime)
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    constexpr float WALK_SPEED{ 1.5f };

    if (!camera->GetViewType()) {
        if (_Right)
            characterPos.x += WALK_SPEED * deltaTime;
        if (_Left)
            characterPos.x -= WALK_SPEED * deltaTime;
        if (_Top)
            characterPos.z -= WALK_SPEED * deltaTime;
        if (_Bottom)
            characterPos.z += WALK_SPEED * deltaTime;
    }
    else {
        glm::vec3 forward(
            sin(camera->GetHorizontalAngle()),
            0,
            cos(camera->GetHorizontalAngle())
        );
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

        glm::vec3 moveDir(0.0f);
        if (_Top) moveDir += forward;
        if (_Bottom) moveDir -= forward;
        if (_Right) moveDir += right;
        if (_Left) moveDir -= right;

        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            characterPos += moveDir * (WALK_SPEED * deltaTime);
        }
    }
}

void Character::Run()
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    if (!camera->GetViewType()) {
        if (_Right)
            characterPos.x += 0.012f;
        if (_Left)
            characterPos.x -= 0.012f;
        if (_Top)
            characterPos.z -= 0.012f;
        if (_Bottom)
            characterPos.z += 0.012f;
    }
    else {
        glm::vec3 forward(
            sin(camera->GetHorizontalAngle()),
            0,
            cos(camera->GetHorizontalAngle())
        );
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

        glm::vec3 moveDir(0.0f);
        if (_Top) moveDir += forward;
        if (_Bottom) moveDir -= forward;
        if (_Right) moveDir += right;
        if (_Left) moveDir -= right;

        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 nextPos = characterPos + moveDir * 0.01f;
            characterPos = nextPos;
        }
    }
}

void Character::Run(float deltaTime)
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    constexpr float RUN_SPEED{ 3.0f };

    if (!camera->GetViewType()) {
        if (_Right)
            characterPos.x += RUN_SPEED * deltaTime;
        if (_Left)
            characterPos.x -= RUN_SPEED * deltaTime;
        if (_Top)
            characterPos.z -= RUN_SPEED * deltaTime;
        if (_Bottom)
            characterPos.z += RUN_SPEED * deltaTime;
    }
    else {
        glm::vec3 forward(
            sin(camera->GetHorizontalAngle()),
            0,
            cos(camera->GetHorizontalAngle())
        );
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

        glm::vec3 moveDir(0.0f);
        if (_Top) moveDir += forward;
        if (_Bottom) moveDir -= forward;
        if (_Right) moveDir += right;
        if (_Left) moveDir -= right;

        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            characterPos += moveDir * (RUN_SPEED * deltaTime);
        }
    }
}

void Character::SaveAnimations()
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

void Character::UpdateAnimation()
{
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

void Character::ChangeCatAnimation(const glm::mat4& view, const glm::mat4& projection)
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    extern double cur_x, cur_y;  // GraphicsManager에서 가져와야 함
    float firetimer;
    glm::vec3 mousePick;

    if (!GetDying())
    {
        if (animLibrary->GetCurrentAnimation() == "FireRun")
        {
            firing_induration = true;
            firetimer = player_CurrentAnim->Duration * 0.56f;

            for (int i = 0; i < 3; ++i)
            {
                if (player_CurrentAnim->CurrentTime >= firetimer + (150.0f * i) && !Bullet_cnt[i])
                {
                    mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
                    Bullet* newBullet = new Bullet(1, 0, 0);
                    newBullet->BulletSetting(this, camera, mousePick);
                    bullets.push_back(newBullet);
                    Bullet_cnt[i] = true;
                }
            }

            if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
            {
                if (!firing)
                {
                    if (IsMoving())
                    {
                        if (Shift_value())
                            animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                        else
                            animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                    }
                    else
                        animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
                    firing_induration = false;
                }
                else
                {
                    if (IsMoving())
                    {
                        if (!Shift_value())
                            animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                        else
                            animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                    }
                    else
                        animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
                }

                for (bool& bullet_nums : Bullet_cnt)
                    bullet_nums = false;
            }
        }
        else if (animLibrary->GetCurrentAnimation() == "FireWalk")
        {
            firing_induration = true;
            firetimer = player_CurrentAnim->Duration * 0.56f;

            for (int i = 0; i < 3; ++i)
            {
                if (player_CurrentAnim->CurrentTime >= firetimer + (150.0f * i) && !Bullet_cnt[i])
                {
                    mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
                    Bullet* newBullet = new Bullet(1, 0, 0);
                    newBullet->BulletSetting(this, camera, mousePick);
                    bullets.push_back(newBullet);
                    Bullet_cnt[i] = true;
                }
            }

            if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
            {
                if (!firing)
                {
                    if (IsMoving())
                    {
                        if (Shift_value())
                            animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                        else
                            animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                    }
                    else
                        animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
                    firing_induration = false;
                }
                else
                {
                    if (IsMoving())
                    {
                        if (Shift_value())
                            animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                        else
                            animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                    }
                    else
                        animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
                }

                for (bool& bullet_nums : Bullet_cnt)
                    bullet_nums = false;
            }
        }
        else if (animLibrary->GetCurrentAnimation() == "Fire")
        {
            firing_induration = true;
            firetimer = player_CurrentAnim->Duration * 0.56f;

            for (int i = 0; i < 3; ++i)
            {
                if (player_CurrentAnim->CurrentTime >= firetimer + (150.0f * i) && !Bullet_cnt[i])
                {
                    mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
                    Bullet* newBullet = new Bullet(1, 0, 0);
                    newBullet->BulletSetting(this, camera, mousePick);
                    bullets.push_back(newBullet);
                    Bullet_cnt[i] = true;
                }
            }

            if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
            {
                if (!firing)
                {
                    if (IsMoving())
                    {
                        if (Shift_value())
                            animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
                        else
                            animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
                    }
                    else
                        animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
                    firing_induration = false;
                }
                else
                {
                    if (IsMoving())
                    {
                        if (Shift_value())
                            animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
                        else
                            animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
                    }
                    else
                        animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
                }

                for (auto& bullet_nums : Bullet_cnt)
                    bullet_nums = false;
            }
        }
        else if (animLibrary->GetCurrentAnimation() == "Die")
            animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
    }
    else
    {
        if (animLibrary->GetCurrentAnimation() != "Die")
            animLibrary->ChangeAnimation("Die", *player_CurrentAnim);
        else
        {
            if (player_CurrentAnim->CurrentTime + 10 >= player_CurrentAnim->Duration)
                SetDead(true);
        }
    }
}

void Character::ThrowBullets(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    for (auto& bullet : bullets)
    {
        bullet->BulletUpdate();
        bullet->Render(orgview, orgproj, viewPos, lightSpaceMatrix, shadowMap);
    }
}

void Character::Setlife()
{
    if (!isLocalPlayer) return;  // 로컬 플레이어만

    if (life > 1)
    {
        life -= 1;
        hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
    }
    else if (life > 0)
    {
        life -= 1;
        hit_cnt = 200;
        hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
        dying = true;
        _Right = { false };
        _Left = { false };
        _Top = { false };
        _Bottom = { false };
        _Shift = { false };
        hitbox_on = { false };
    }
}

void Character::SetAnimationType(const std::string& animName)
{
    if (animLibrary && player_CurrentAnim) {
        animLibrary->ChangeAnimation(animName, *player_CurrentAnim);
    }
}

void Character::UpdateFromPacket(float x, float y, float z, char direction, bool run)
{
    SetTargetPosition(x, y, z);
    currentDirection = direction;
    isRunning = run;
}

void Character::SetTargetPosition(float x, float y, float z)
{
    targetPos = glm::vec3(x, y, z);
}