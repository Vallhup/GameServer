#pragma once
#include "AnimatedModel.h"

class Bullet;
class MainCharacter;
class ShadowMapping;

class Enemy
{
public:
    Enemy(int num, int POINT);
    ~Enemy();

    void LoadModel();
    void SetSpawnPosition();
    void SetSpawnAngle();
    void SetupShaders();
    void Update(float deltaTime, const glm::vec3& cPos, Enemy* enemy, MainCharacter* mainCat);

    void Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint depthMap);

    void DrawEnemyShadow(ShadowMapping* shadowMap);
    void DrawEnemyBulletShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);

    void DrawAttackingLine(const glm::mat4& view, const glm::mat4& projection);

    void MoveToward(MainCharacter* mainCat);
    void RotateEnemy(const glm::vec3& cPos, Enemy* enemy);
    void LookUpdate(const glm::vec3& cPos);
    void MakeBullets(const glm::vec3& cPos);

    bool wallcollapsed_s();
    bool wallcollapsed_w();
    bool wallcollapsed_d();
    bool wallcollapsed_a();

    void UpdateStateAndBehavior(MainCharacter* mainCat);

    void SetLife();

    const glm::vec3& GetPosition() const { return enemypos; }
    const glm::mat4& GetModel() const { return model; }
    const int& GetState() const { return state; }
    const int& GetLife() const { return life; }
    const bool& GetDead() const { return dead; }
    const float& GetLastAngle() const { return lastangle; }

    void ThrowBullets(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);

    void SetDead();
    void SetReviveTimer();

    void SaveAnimations();
    void ChangeEnemyAnimation();
    void ChangeHitColor();

    void ReviveEnemy(const glm::vec3& cPos);

private:
    glm::vec3 enemypos, targetpos;
    bool aim_target{ false };
    GLuint VAO, VBO, VBO2, EBO, shaderprogram, Texture;
    GLuint ViewLoc, ProjLoc, ModelLoc, TextureLoc, UseTextureLoc, colorHitLoc;
    GLuint lVAO, lVBO, shaderprogram2;
    std::vector<unsigned int> Indices;
    glm::mat4 model{ 1.0f };
    glm::vec4 hitcolor{ 1.0f, 1.0f, 1.0f, 1.0f };
    float lastangle;
    int type{ 0 };
    int point{ -1 };
    int state{ 0 };     // 0: Idle, 1: Run, 2:Attack, 3: Hit, 4: Die, 5: Dance
    int life{ 6 };
    int hit_cnt{ 200 };
    float revive_timer{ 2400.0f };   // 240Hz 모니터 기준 10초
    bool dead{ false };
    bool fire{ false };
    bool newbullet{ false };

    Bullet* newBullet;
    glm::vec3 tPos;
    vector<Bullet*> bullets;

    vector<BoneInfo>* alien_BoneInfo;
    AnimatedModel* animModel;
    AnimInfo* enemy_CurrentAnim;
    AnimatedModel::AnimationLibrary* animLibrary = { nullptr };

    std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;
};