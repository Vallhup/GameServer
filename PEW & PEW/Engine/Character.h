#pragma once
#include "AnimatedModel.h"

class Bullet;
class Camera;
class BoundingBox;

class Character
{
public:
    Character(int id, bool isLocal = false);
    ~Character();

    void Init();
    void Update(float deltaTime);
    void Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime);
    void DrawShadow(GLuint depthShaderProgram, const glm::mat4& lightSpaceMatrix);
    void DrawBulletShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);

    // 입력 처리 (로컬 플레이어만)
    void SetRight_on(bool in) { if (isLocalPlayer) _Right = in; }
    void SetLeft_on(bool in) { if (isLocalPlayer) _Left = in; }
    void SetTop_on(bool in) { if (isLocalPlayer) _Top = in; }
    void SetBottom_on(bool in) { if (isLocalPlayer) _Bottom = in; }
    void Shift_on(bool in) { if (isLocalPlayer) _Shift = in; }
    void hitboxOnOff(bool in) { if (isLocalPlayer) hitbox_on = in; }
    void SetFiring(bool in) { if (isLocalPlayer) firing = in; }
    void SetCamera(Camera* cam) { if (isLocalPlayer) camera = cam; }
    void SetAngle(float ang) { angle = ang; }

    // 네트워크 업데이트 (원격 플레이어만)
    void UpdateFromPacket(float ang, float x, float y, float z, char direction = -1, bool run = false);
    void SetTargetPosition(float x, float y, float z);

    // Getter
    int GetPlayerID() const { return playerID; }
    bool IsLocalPlayer() const { return isLocalPlayer; }
    glm::vec3 GetPosition() const { return characterPos; }
    const glm::mat4& GetModel() const { return model; }
    bool IsMoving() const { return _Right || _Left || _Top || _Bottom; }
    bool Shift_value() const { return _Shift; }
    bool GetFiring() const { return firing; }
    bool GetFiringInduration() const { return firing_induration; }
    bool GetDying() const { return dying; }
    bool GetRight() const { return _Right; }
    bool GetLeft() const { return _Left; }
    bool GetTop() const { return _Top; }
    bool GetBottom() const { return _Bottom; }
    bool hitbox_ison() const { return hitbox_on; }
    float GetAngle() const { return angle; }

    // MainCharacter 기존 함수들 (로컬 플레이어만)
    void ChangeCatAnimation(const glm::mat4& view, const glm::mat4& projection);
    void ThrowBullets(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
    void SetAnimationType(const std::string& animName);
    AnimInfo* GetCurrentAnim() { return player_CurrentAnim; }
    AnimatedModel::AnimationLibrary* GetAnimLibrary() { return animLibrary; }
    void Setlife();
    void SetPosition() { if (isLocalPlayer) characterPos = glm::vec3{ -44.0f, 0.0f, -48.0f }; }
    void SetDead(bool in) { if (isLocalPlayer) dead = in; }
    void SetFinishPos() { if (isLocalPlayer) characterPos = glm::vec3{ -5.0f, 0.0f, 5.0f }; }

private:
    // 기본 정보
    int playerID;
    bool isLocalPlayer;

    // 공통 렌더링 관련 (MainCharacter + RemotePlayer 통합)
    vector<BoneInfo>* player_BoneInfo;
    AnimatedModel* animModel;
    AnimInfo* player_CurrentAnim;
    AnimatedModel::AnimationLibrary* animLibrary;
    std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;

    // OpenGL 오브젝트들
    GLuint VAO, VBO, VBO2, EBO, shaderprogram, Texture;
    GLuint ViewLoc, ProjLoc, ModelLoc, TextureLoc, UseTextureLoc, colorHitLoc;
    std::vector<unsigned int> Indices;
    glm::mat4 model;
    glm::vec4 hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // 위치 및 이동
    glm::vec3 characterPos;
    glm::vec3 targetPos;          // 원격 플레이어용 보간 목표
    float lerpSpeed = 15.0f;      // 원격 플레이어용 보간 속도

    // 입력 상태
    bool _Right = false, _Left = false, _Top = false, _Bottom = false;
    bool _Shift = false;
    bool hitbox_on = false;

    // 게임 상태 (로컬 플레이어만 사용)
    bool dead = false, dying = false;
    int life = 6;
    float revive_timer = 1200.0f;
    int hit_cnt = 200;
    float angle;
    float lastangle;

    // 전투 관련 (로컬 플레이어만)
    bool firing = false;
    bool firing_induration = false;
    bool Bullet_cnt[3] = { false, false, false };
    vector<Bullet*> bullets;

    // 로컬 플레이어 전용
    Camera* camera = nullptr;
    BoundingBox* hitbox = nullptr;

    // 원격 플레이어 전용
    char currentDirection = -1;
    bool isRunning = false;

    // 공통 함수들
    void SaveAnimations();
    void UpdateAnimation();
    void Walk();
    void Walk(float deltaTime);
    void Run();
    void Run(float deltaTime);
    void HandleLocalPlayerUpdate(float deltaTime);
    void HandleRemotePlayerUpdate(float deltaTime);
};