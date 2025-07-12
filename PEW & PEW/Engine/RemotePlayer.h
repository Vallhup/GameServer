#pragma once
#include "AnimatedModel.h"

class RemotePlayer
{
public:
    RemotePlayer(int id);
    ~RemotePlayer();

    void Init();
    void Update(float deltaTime);
    void Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime);
    void DrawShadow(GLuint depthShaderProgram, const glm::mat4& lightSpaceMatrix);

    // 네트워크 업데이트
    void UpdateFromPacket(float x, float y, float z, char direction = -1, bool run = false);
    void SetTargetPosition(float x, float y, float z);

    // Getter
    int GetPlayerID() const { return playerID; }
    glm::vec3 GetPosition() const { return characterPos; }

private:
    void SaveAnimations();
    void UpdateAnimation();

private:
    int playerID;

    // 캐릭터 렌더링 관련 (MainCharacter와 동일한 구조)
    vector<BoneInfo>* player_BoneInfo;
    AnimatedModel* animModel;
    AnimInfo* player_CurrentAnim;
    AnimatedModel::AnimationLibrary* animLibrary;

    // OpenGL 오브젝트들
    GLuint VAO, VBO, VBO2, EBO;
    GLuint shaderprogram;
    GLuint Texture;
    GLuint ViewLoc, ProjLoc, ModelLoc, TextureLoc, UseTextureLoc;
    vector<GLuint> Indices;
    glm::mat4 model;

    // 위치 및 애니메이션 정보
    glm::vec3 characterPos;
    glm::vec3 targetPos;
    char currentDirection;
    bool isRunning;

    // 보간을 위한 변수들
    float lerpSpeed = 15.0f;

    // 시각적 효과
    glm::vec4 hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;
};