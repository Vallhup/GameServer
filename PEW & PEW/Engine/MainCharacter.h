#pragma once
#include "AnimatedModel.h"

class Bullet;
class Camera;
class BoundingBox;
class AlienCharacter;
class SceneManager;

struct CatBulletSlot {
    Bullet* bullet = nullptr;
    int bulletID;           // 네트워크 동기화용 ID
    bool isActive;

    CatBulletSlot() : bulletID(-1), isActive(false) {}
};

class MainCharacter
{
public:
    MainCharacter(int id, bool isLocal = false, float speed = 0.1f);
    ~MainCharacter();

    void Init();
    void Update(float deltaTime);
    void Update(float deltaTime, array<array<AlienCharacter*, 9>, 3>& aliens);
    void Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime, glm::mat4 lightSpaceMatrix, GLuint depthMap);
    void DrawShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShaderProgram);
    void RenderBullets(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
    void RenderBulletsShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);

    // 서버 함수
    void CreateBulletFromServer(int bulletID, glm::vec3 startPos);
    bool SetBulletNextPosFromServer(int bulletID, glm::vec3 newPos);
    bool RemoveBulletFromServer(int bulletID);
    void UpdateBulletsFromServer(float deltaTime);

    // 입력 처리 (로컬 플레이어만)
    void SetRight(bool in) { if (isLocalPlayer) _Right = in; }
    void SetLeft(bool in) { if (isLocalPlayer) _Left = in; }
    void SetTop(bool in) { if (isLocalPlayer) _Top = in; }
    void SetBottom(bool in) { if (isLocalPlayer) _Bottom = in; }
    void SetShift(bool in) { if (isLocalPlayer) _Shift = in; }
    void SetHitBox(bool in) { if (isLocalPlayer) hitbox_on = in; }
    void SetFiring(bool in) { firing = in; }
    void SetCamera(Camera* cam) { if (isLocalPlayer) camera = cam; }
    void SetAngle(float ang) { angle = ang; }

    // 네트워크 업데이트 (원격 플레이어만)
    void UpdateFromPacket(float ang, float x, float y, float z, char direction = -1, bool move = false, bool run = false);
    void ReviveFromPacket(float x, float y, float z);
    void DamagedFromPacket();
    void SetTargetPosition(float x, float y, float z);

    // 애니메이션
    void SaveAnimations();
    void UpdateAnimation(float deltaTime);

    // 로컬 (Scene1) 전용
    void UpdateLocalPlayerMovement(float deltaTime);
    void UpdateLocalPlayerState();
    void LocalMove(float deltaTime);
    void UpdateLocalBullets(array<array<AlienCharacter*, 9>, 3>& aliens);
    void CreateLocalBullet();
    void CheckFireAnimationTiming();
    void CheckBulletAlienHit(int bulletIndex, array<array<AlienCharacter*, 9>, 3>& aliens);
    void UpdateLocalPlayerRevive();
    void CheckLocalEnd(array<array<AlienCharacter*, 9>, 3>& aliens);
    void ResetAllStates();
    void GoToEndPosition() { characterPos = glm::vec3(-45.0f, 0.0f, -40.0f); }  // 잠시 사용하기 위해 만든 함수

    // 공통 함수
    void UpdateAllPlayersMovement(float deltaTime);
    void UpdateHitDecision();
    void SetSceneManager(SceneManager* sm) { sceneManager = sm; }

    // Getter
    bool CheckLocal() const { return isLocalPlayer; }
    glm::vec3 GetPosition() const { return characterPos; }
    bool IsMoving() const { return _Right || _Left || _Top || _Bottom; }
    bool GetShift() const { return _Shift; }
    bool GetDying() const { return dying; }
    bool GetDead() const { return dying || dead; }  // Alien들과 상호작용을 위한 함수
    bool GetRight() const { return _Right; }
    bool GetLeft() const { return _Left; }
    bool GetTop() const { return _Top; }
    bool GetBottom() const { return _Bottom; }
    bool GetHitBox() const { return hitbox_on; }
    float GetAngle() const { return angle; }
    AnimInfo* GetCurrentAnim() { return player_CurrentAnim; }
    AnimatedModel::AnimationLibrary* GetAnimLibrary() { return animLibrary; }

    // Setter
    void SetDying(bool in) { dying = in; }
    void SetDead(bool in) { dead = in; }
    void SetHit();

private:
    // 기본 정보
    int playerID;
    bool isLocalPlayer;
    glm::vec3 characterPos;
    glm::vec3 targetPos;          // 원격 플레이어용 보간 목표
    float lerpSpeed = { 15.0f };      // 원격 플레이어용 보간 속도

    // 캐릭터 상태
    bool dying = { false }, dead = { false };
    int hit_cnt = { 0 };
    bool firing = { false };
    int life = { 5 };       // local life
    int reviveCount = { 300 };

    // 입력
    bool _Right = { false }, _Left = { false }, _Top = { false }, _Bottom = { false };
    bool _Shift = { false };
    bool hitbox_on = { false };

    // 움직임
    float angle;
    char currentDirection = { -1 };
    bool isMoving = { false };
    bool isRunning = { false };

    // 총알
    static const int MAX_BULLETS = { 15 };  // 캐릭터당 최대 총알 수
    array<CatBulletSlot, MAX_BULLETS> bullets;
    bool localBulletFired[3] = { false, false, false };

    // 씬 전환
    SceneManager* sceneManager = nullptr;

    // 로컬 플레이어 전용
    Camera* camera = { nullptr };
    BoundingBox* hitbox = { nullptr };

    // 모델 + 애니메이션
    vector<BoneInfo>* player_BoneInfo;
    AnimatedModel* animModel;
    AnimInfo* player_CurrentAnim;
    AnimatedModel::AnimationLibrary* animLibrary;
    std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;

    // OpenGL
    GLuint VAO, VBO, VBO2, EBO, shaderprogram, Texture;
    GLuint ViewLoc, ProjLoc, ModelLoc, TextureLoc, UseTextureLoc, colorHitLoc;
    std::vector<unsigned int> Indices;
    glm::mat4 model;
    glm::vec4 hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};