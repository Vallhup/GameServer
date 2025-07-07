#include "pch.h"
#include "Enemy.h"
#include "MainCharacter.h"
#include "Bullet.h"
#include "ShadowMapping.h"

Enemy::Enemy(int num, int POINT)
{
    alien_BoneInfo = new vector<BoneInfo>();
	animModel = new AnimatedModel();
    enemy_CurrentAnim = new AnimInfo();
    animLibrary = new AnimatedModel::AnimationLibrary();

    type = num;
    point = POINT;

    SaveAnimations();
    LoadModel();
    SetSpawnPosition();
    SetSpawnAngle();
    SetupShaders();

    glGenVertexArrays(1, &lVAO);
    glGenBuffers(1, &lVBO);
}

Enemy::~Enemy()
{
    delete enemy_CurrentAnim;
    delete alien_BoneInfo;
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

void Enemy::LoadModel()
{
    if (type == 1)
    {
        animModel->LoadGLBFile(type + 1, *alien_BoneInfo, "Glb/alien_1_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
        Texture = LoadTexture("Texture/alien_1_basecolor.png");
    }
    else if (type == 2)
    {
        animModel->LoadGLBFile(type + 1, *alien_BoneInfo, "Glb/alien_2_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
        Texture = LoadTexture("Texture/alien_2_basecolor.png");
    }
    else if (type == 3)
    {
        animModel->LoadGLBFile(type + 1, *alien_BoneInfo, "Glb/alien_3_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
        Texture = LoadTexture("Texture/alien_3_basecolor.png");
    }
}

void Enemy::SetSpawnPosition()
{
    const std::vector<glm::vec3> ENEMY_SPAWN_POINTS = {
    glm::vec3(1.54972, 0.0f, 35.1811),
    glm::vec3(-2.91028, 0.0f, 19.5118),
    glm::vec3(9.50484, 0.0f, 10.4216),
    glm::vec3(26.1052, 0.0f, 21.8918),
    glm::vec3(36.5146, 0.0f, -33.659),
    glm::vec3(12.7399, 0.0f, -32.5541),
    glm::vec3(-7.1254, 0.0f, -45.7119),
    glm::vec3(-16.6856, 0.0f, -28.7741),
    glm::vec3(-48.3682, 0.0f, -19.8239),

    glm::vec3(0.0496913, 0.0f, 36.1712),
    glm::vec3(-1.64031, 0.0f, 18.1016),
    glm::vec3(10.5848, 0.0f, 11.7714),
    glm::vec3(24.5802, 0.0f, 23.1417),
    glm::vec3(37.8644, 0.0f, -32.0894),
    glm::vec3(14.025, 0.0f, -34.249),
    glm::vec3(-6.04527, 0.0f, -44.4673),
    glm::vec3(-15.4905, 0.0f, -30.1794),
    glm::vec3(-47.3833, 0.0f, -21.1092),

    glm::vec3(1.54463, 0.0f, 37.201),
    glm::vec3(-2.87536, 0.0f, 16.5065),
    glm::vec3(11.4648, 0.0f, 10.4414),
    glm::vec3(26.0501, 0.0f, 24.3717),
    glm::vec3(39.3223, 0.0f, -33.7221),
    glm::vec3(12.8531, 0.0f, -36.0768),
    glm::vec3(-4.94215, 0.0f, -45.7352),
    glm::vec3(-16.5574, 0.0f, -31.8925),
    glm::vec3(-48.2449, 0.0f, -22.5823),
    };

    enemypos = ENEMY_SPAWN_POINTS[(9 * (type - 1)) + point];
}

void Enemy::SetSpawnAngle()
{
    switch (point)
    {
    case 0:
        lastangle = -0.52f;
        break;
    case 1:
        lastangle = 2.6f;
        break;
    case 2:
        lastangle = 1.03f;
        break;
    case 3:
        lastangle = -0.52f;
        break;
    case 4:
        lastangle = 1.03f;
        break;
    case 5:
        lastangle = 2.6f;
        break;
    case 6:
        lastangle = 1.03f;
        break;
    case 7:
        lastangle = 2.6f;
        break;
    case 8:
        lastangle = 2.6f;
        break;
    }
}

void Enemy::SetupShaders()
{
    SetupShader("Shaders/EnemyVert.glsl", "Shaders/EnemyFrag.glsl", shaderprogram);
    SetupShader("Shaders/EnemyLineVert.glsl", "Shaders/EnemyLineFrag.glsl", shaderprogram2);
}

void Enemy::Update(float deltaTime, const glm::vec3& cPos, Enemy* enemy, MainCharacter* mainCat)
{
    if (!dead)
    {
        animModel->UpdateAnimation(type + 1, *alien_BoneInfo, deltaTime, *enemy_CurrentAnim);
        RotateEnemy(cPos, enemy);
        ChangeEnemyAnimation();
        UpdateStateAndBehavior(mainCat);
        MoveToward(mainCat);
        MakeBullets(cPos);
    }
    else
        ReviveEnemy(cPos);
}

void Enemy::Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint depthMap)
{
    if (!dead)
    {
        glUseProgram(shaderprogram);
        animModel->SetupBoneTransforms(*alien_BoneInfo, shaderprogram);

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

        if (state == 2 && enemy_CurrentAnim->CurrentTime < 750)
        {
            DrawAttackingLine(view, projection);
        }

        ChangeHitColor();
    }
}

void Enemy::DrawEnemyShadow(ShadowMapping* shadowMap)
{
    if (!dead)
    {
        ModelLoc = glGetUniformLocation(shadowMap->GetDepthShaderProgram(), "model");
        glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        animModel->SetupBoneTransforms(*alien_BoneInfo, shadowMap->GetDepthShaderProgram());
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Enemy::DrawEnemyBulletShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
    for (auto& bullet : bullets)
    {
        bullet->RenderShadow(lightSpaceMatrix, depthShader);
    }
}

void Enemy::DrawAttackingLine(const glm::mat4& view, const glm::mat4& projection)
{
    glUseProgram(shaderprogram2);

    const int segments = 30;
    std::vector<glm::vec3> linePositions;

    glm::vec3 startPos = { enemypos.x, 0.45f, enemypos.z };
    glm::vec3 endPos = { targetpos.x - (enemypos.x - targetpos.x), 0.45f,
                        targetpos.z - (enemypos.z - targetpos.z) };

    glm::vec3 direction = endPos - startPos;
    float totalLength = glm::length(direction);
    float segmentLength = totalLength / (segments * 2);

    direction = glm::normalize(direction);

    for (int i = 0; i < segments; i++) {
        float start = i * segmentLength * 2;
        linePositions.push_back(startPos + direction * start);
        linePositions.push_back(startPos + direction * (start + segmentLength));
    }

    glBindVertexArray(lVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lVBO);
    glBufferData(GL_ARRAY_BUFFER, linePositions.size() * sizeof(glm::vec3),
        linePositions.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUniformMatrix4fv(glGetUniformLocation(shaderprogram2, "view"), 1,
        GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderprogram2, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shaderprogram2, "lineColor"), 1.0f, 0.2f, 0.2f);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, linePositions.size());
    glBindVertexArray(0);
}

void Enemy::MoveToward(MainCharacter* mainCat)
{
    if (!dead && !mainCat->GetDying())
    {
        glm::vec3 pos = mainCat->GetPosition();
        glm::vec3 direction = glm::normalize(pos - enemypos);
        float distance = glm::length(glm::vec2(pos.x - enemypos.x, pos.z - enemypos.z));

        if (distance > 4.0f && distance < 13.0f)
        {
            if (state != 2 && state != 4)
            {
                if (/*!wallcollapsed_s() &&*/ pos.z > enemypos.z)
                    enemypos.z += 0.01f;
                if (/*!wallcollapsed_w() &&*/ pos.z < enemypos.z)
                    enemypos.z -= 0.01f;
                if (/*!wallcollapsed_d() &&*/ pos.x > enemypos.x)
                    enemypos.x += 0.01f;
                if (/*!wallcollapsed_a() &&*/ pos.x < enemypos.x)
                    enemypos.x -= 0.01f;
            }

            if (state == 0)
                state = 1;
        }
        else if (distance <= 4.0f)
        {
            if (!aim_target)
            {
                targetpos = mainCat->GetPosition();
                aim_target = true;
            }

            if (state != 4)
            {
                state = 2;
            }
        }
        else if (distance >= 13.0f)
        {
            if (state == 1)
            {
                state = 0;
                lastangle += 0.785f;
            }
        }
    }
}

void Enemy::RotateEnemy(const glm::vec3& cPos, Enemy* enemy)
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, enemy->GetPosition());
    glm::vec3 pos = cPos;
    float distance = glm::length(glm::vec2(pos.x - enemypos.x, pos.z - enemypos.z));
    float angle = atan2(pos.x - enemypos.x, pos.z - enemypos.z);
    if (distance < 13.0f)
    {
        if (!(state == 2 || state == 4))
        {
            lastangle = angle;
        }
    }
    model = glm::rotate(model, lastangle, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Enemy::LookUpdate(const glm::vec3& cPos)
{
    glm::vec3 pos = cPos;
    float distance = glm::length(glm::vec2(pos.x - enemypos.x, pos.z - enemypos.z));
    float angle = atan2(pos.x - enemypos.x, pos.z - enemypos.z);

    model = glm::mat4(1.0f);
    model = glm::translate(model, enemypos);
    if (distance > 4.0f && distance < 13.0f)
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Enemy::MakeBullets(const glm::vec3& cPos)
{
    if (state == 2 && enemy_CurrentAnim->CurrentTime < 750)
    {
        if (!newbullet)
        {
            newBullet = new Bullet(2, type - 1, point);
            newBullet->BulletSetting(this, cPos);
            newbullet = true;

            tPos = newBullet->GettPos();
        }
    }
}

//bool Enemy::wallcollapsed_s()
//{
//    glm::vec3 pos = enemypos;
//    bool check = false;
//    glm::vec3 other_pos;
//
//    for (int i = 0; i < 70; ++i)
//    {
//        if (min_X[i] <= pos.x && max_X[i] >= pos.x)
//        {
//            if (pos.z + 0.25f <= max_Z[i] && pos.z + 0.25f > min_Z[i])
//                check = true;
//        }
//    }
//
//    for (int i = 0; i < 3; ++i)
//    {
//        for (int j = 0; j < 9; ++j)
//        {
//            if (i == (type - 1) && j == point)
//                continue;
//            else
//            {
//                if (!(enemy[i][j]->getstate() == 4))
//                {
//                    other_pos = enemy[i][j]->getPosition();
//                    if (abs(pos.x - other_pos.x) < 0.6f)
//                    {
//                        if (other_pos.z - pos.z < 0.6f && other_pos.z - pos.z > 0.0f)
//                            check = true;
//                    }
//                }
//            }
//        }
//    }
//
//    return check;
//}
//
//bool Enemy::wallcollapsed_w()
//{
//    glm::vec3 pos = enemypos;
//    bool check = false;
//    glm::vec3 other_pos;
//
//    for (int i = 0; i < 70; ++i)
//    {
//        if (min_X[i] <= pos.x && max_X[i] >= pos.x)
//        {
//            if (pos.z - 0.25f <= max_Z[i] && pos.z - 0.25f > min_Z[i])
//                check = true;
//        }
//    }
//
//    for (int i = 0; i < 3; ++i)
//    {
//        for (int j = 0; j < 9; ++j)
//        {
//            if (i == (type - 1) && j == point)
//                continue;
//            else
//            {
//                if (!(enemy[i][j]->getstate() == 4))
//                {
//                    other_pos = enemy[i][j]->getPosition();
//                    if (abs(pos.x - other_pos.x) < 0.6f)
//                    {
//                        if (other_pos.z - pos.z > -0.6f && other_pos.z - pos.z < 0.0f)
//                            check = true;
//                    }
//                }
//            }
//        }
//    }
//
//    return check;
//}
//
//bool Enemy::wallcollapsed_d()
//{
//    glm::vec3 pos = enemypos;
//    bool check = false;
//    glm::vec3 other_pos;
//
//    for (int i = 0; i < 70; ++i)
//    {
//        if (min_Z[i] <= pos.z && max_Z[i] >= pos.z)
//        {
//            if (pos.x + 0.25f <= max_X[i] && pos.x + 0.25f > min_X[i])
//                check = true;
//        }
//    }
//
//    for (int i = 0; i < 3; ++i)
//    {
//        for (int j = 0; j < 9; ++j)
//        {
//            if (i == (type - 1) && j == point)
//                continue;
//            else
//            {
//                if (!(enemy[i][j]->getstate() == 4))
//                {
//                    other_pos = enemy[i][j]->getPosition();
//                    if (abs(pos.z - other_pos.z) < 0.6f)
//                    {
//                        if (other_pos.x - pos.x < 0.6f && other_pos.x - pos.x > 0.0f)
//                            check = true;
//                    }
//                }
//            }
//        }
//    }
//
//    return check;
//}
//
//bool Enemy::wallcollapsed_a()
//{
//    glm::vec3 pos = enemypos;
//    bool check = false;
//    glm::vec3 other_pos;
//
//    for (int i = 0; i < 70; ++i)
//    {
//        if (min_Z[i] <= pos.z && max_Z[i] >= pos.z)
//        {
//            if (pos.x - 0.25f <= max_X[i] && pos.x - 0.25f > min_X[i])
//                check = true;
//        }
//    }
//
//    for (int i = 0; i < 3; ++i)
//    {
//        for (int j = 0; j < 9; ++j)
//        {
//            if (i == (type - 1) && j == point)
//                continue;
//            else
//            {
//                if (!(enemy[i][j]->getstate() == 4))
//                {
//                    other_pos = enemy[i][j]->getPosition();
//                    if (abs(pos.z - other_pos.z) < 0.6f)
//                    {
//                        if (other_pos.x - pos.x > -0.6f && other_pos.x - pos.x < 0.0f)
//                            check = true;
//                    }
//                }
//            }
//        }
//    }
//
//    return check;
//}

void Enemy::UpdateStateAndBehavior(MainCharacter* mainCat)
{
    glm::vec3 pos = mainCat->GetPosition();
    glm::vec3 direction = glm::normalize(pos - enemypos);
    float distance = glm::length(glm::vec2(pos.x - enemypos.x, pos.z - enemypos.z));

    if (state == 1)
    {
        if (mainCat->GetDying())
            state = 0;
    }
    else if (state == 2)
    {
        if ((enemy_CurrentAnim->CurrentTime >= 800 && enemy_CurrentAnim->CurrentTime <= 1300) && !fire)
        {
            newBullet = new Bullet(2, type - 1, point);
            newBullet->BulletSettingAgain(this, tPos);
            bullets.push_back(newBullet);
        }
        else if (enemy_CurrentAnim->CurrentTime > 1300 && !fire)
        {
            fire = true;
        }
        else if (enemy_CurrentAnim->CurrentTime + 10 >= (enemy_CurrentAnim->Duration))
        {
            if (mainCat->GetDying())
                state = 0;
            {
                if (distance > 4.0f && distance < 13.0f)
                {
                    state = 1;
                }
                else if (distance >= 13.0f)
                {
                    state = 0;
                }
                else
                {
                    float angle = atan2(pos.x - enemypos.x, pos.z - enemypos.z);
                    if (angle != lastangle)
                    {
                        lastangle = angle;
                    }
                }
                enemy_CurrentAnim->CurrentTime = 0;
                aim_target = false;
                fire = false;
                newbullet = false;
            }

            for (auto& bullet : bullets)
            {
                delete bullet;  // 메모리 해제
            }
            bullets.clear();  // 벡터 비우기
        }
    }
    else if (state == 3)
    {
        if (enemy_CurrentAnim->CurrentTime + 10 >= (enemy_CurrentAnim->Duration))
        {
            if (mainCat->GetDying())
                state = 0;
            else
            {
                if (distance > 4.0f && distance < 13.0f)
                {
                    state = 1;
                }
                else if (distance >= 13.0f)
                {
                    state = 0;

                }
            }
        }
    }
    else if (state == 4)
    {
        if (enemy_CurrentAnim->CurrentTime + 10 >= (enemy_CurrentAnim->Duration))
        {
            dead = true;
        }
    }
}

void Enemy::SetLife()
{
    if (life > 1)
    {
        life -= 1;
        hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
        if (state != 2)
        {
            state = 3;
        }
    }
    else if (life > 0)
    {
        life -= 1;
        hit_cnt = 200;
        hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
        state = 4;
    }
}

void Enemy::ThrowBullets(const glm::mat4& view, const glm::mat4& projection,
    glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
    for (auto& bullet : bullets)
    {
        bullet->BulletUpdate();
        bullet->Render(view, projection, viewPos, lightSpaceMatrix, shadowMap);
    }
}

void Enemy::SetDead()
{
    state = 4;
    life = 0;
}

void Enemy::SetReviveTimer()
{
    revive_timer = 240.0f;
}

void Enemy::SaveAnimations()
{
    animLibrary->LoadAnimation("Idle", "Animations/alien_animation_idle.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Run", "Animations/alien_animation_run.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Attack", "Animations/alien_animation_attack.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Hit", "Animations/alien_animation_hit.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Death", "Animations/alien_animation_death.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Dance", "Animations/cat_animation_dance.glb", animationImporters, animModel);

    animLibrary->ChangeAnimation("Idle", *enemy_CurrentAnim);
}

void Enemy::ChangeEnemyAnimation()
{
    if (state == 0)
    {
        if (animLibrary->GetCurrentAnimation() != "Idle")
            animLibrary->ChangeAnimation("Idle", *enemy_CurrentAnim);
    }
    else if (state == 1)
    {
        if (animLibrary->GetCurrentAnimation() != "Run")
            animLibrary->ChangeAnimation("Run", *enemy_CurrentAnim);
    }
    else if (state == 2)
    {
        if (animLibrary->GetCurrentAnimation() != "Attack")
            animLibrary->ChangeAnimation("Attack", *enemy_CurrentAnim);
    }
    else if (state == 3)
    {
        if (animLibrary->GetCurrentAnimation() == "Idle")
            animLibrary->ChangeAnimation("Hit", *enemy_CurrentAnim);
    }
    else if (state == 4)
    {
        if (animLibrary->GetCurrentAnimation() != "Death")
            animLibrary->ChangeAnimation("Death", *enemy_CurrentAnim);
    }
    else if (state == 5)
    {
        if (animLibrary->GetCurrentAnimation() != "Dance")
            animLibrary->ChangeAnimation("Dance", *enemy_CurrentAnim);
    }
}

void Enemy::ChangeHitColor()
{
    if (hitcolor == glm::vec4(1.0f, 0.6f, 0.6f, 1.0f))
    {
        hit_cnt -= 1;
    }

    if (hit_cnt == 0)
    {
        hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        hit_cnt = 200;
    }
}

void Enemy::ReviveEnemy(const glm::vec3& cPos)
{
    if (dead && revive_timer > 0)
    {
        revive_timer -= 1.0f;
    }
    else if (dead && revive_timer == 0)
    {
        SetSpawnPosition();
        state = { 0 };
        life = { 6 };
        hit_cnt = { 200 };
        revive_timer = { 2400.0f };
        dead = { false };
        aim_target = { false };
        glm::vec3 pos = cPos;
        lastangle = atan2(pos.x - enemypos.x, pos.z - enemypos.z);
        fire = { false };
        newbullet = { false };
        tPos = glm::vec3(0.0f);

        glm::vec3 direction = glm::normalize(pos - enemypos);
        float distance = glm::length(glm::vec2(pos.x - enemypos.x, pos.z - enemypos.z));

        if (distance > 4.0f)
        {
            SetSpawnAngle();
        }
        else
            model = glm::rotate(model, lastangle, glm::vec3(0.0f, 1.0f, 0.0f));

        //if (finish)
        //    state = 5;
    }
}
