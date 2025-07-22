#pragma once
#include "AnimatedModel.h"

class MainCharacter;
class Bullet;
class ShadowMapping;

struct AlienBulletSlot {
	Bullet* bullet = nullptr;
	bool isActive;

	AlienBulletSlot() : isActive(false) {}
};

class AlienCharacter
{
public:
	AlienCharacter(int type, int location);
	~AlienCharacter();

	void Update(float deltaTime, MainCharacter* Cat);

	void Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime, glm::mat4 lightSpaceMatrix, GLuint depthMap);
	void DrawShadow(ShadowMapping* shadowMap);
	void DrawBullets(const glm::mat4& view, const glm::mat4& projection, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void DrawBulletsShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);
	void DrawAttackingLine(const glm::mat4& view, const glm::mat4& projection);

	void SaveAnimations();
	void LoadModel();
	void SetSpawnPosition();
	void SetSpawnAngle();
	void SetupShaders();

	void RotateAliens(MainCharacter* Cat);
	void ChangeAnimation(float deltaTime);
	void UpdateStateAndBehavior(MainCharacter* Cat);
	void MoveToward(MainCharacter* Cat);

	void ActivateBullets();
	void DeactivateBullets();
	void UpdateBullets(MainCharacter* Cat);

	void UpdateHitDecision();
	void SetHit();
	void SetDying() { dying = true; }

	const glm::vec3& GetPosition() const { return alienPos; }
	bool GetDying() const { return dying; }
	bool GetDead() const { return dead; }

private:
	// 적 정보
	glm::vec3 alienPos;
	glm::vec3 targetPos;
	float viewingAngle;
	int state = { 0 };     // 0: Idle, 1: Run, 2:Attack, 3: Hit, 4: Die
	int life = { 3 };
	bool dead = { false }, dying = { false };
	bool shotFired[10] = { false };

	glm::vec4 hitcolor = { 1.0f, 1.0f, 1.0f, 1.0f };
	int hit_cnt = { 200 };

	// 적 종류와 위치
	int alienType;
	int alienLocationSetter;

	static const int MAX_BULLETS = { 10 };  // 적 하나당 최대 총알 수
	array<AlienBulletSlot, MAX_BULLETS> bullets;

	// 애니메이션
	vector<BoneInfo>* alien_BoneInfo;
	AnimatedModel* animModel;
	AnimInfo* alien_CurrentAnim;
	AnimatedModel::AnimationLibrary* animLibrary = { nullptr };
	std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;

	// 캐릭터 OPENGL
	glm::mat4 model = glm::mat4(1.0f);
	GLuint aVAO, aVBO, aVBO2, aEBO, aTexture, aShaderprogram;
	GLuint ViewLoc, ProjLoc, ModelLoc, TextureLoc, UseTextureLoc, colorHitLoc;
	std::vector<unsigned int> aIndices;

	// 공격선 OPENGL
	GLuint lVAO, lVBO, lShaderprogram;
};

