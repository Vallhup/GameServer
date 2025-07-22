#pragma once
#include "AnimatedModel.h"

class AlienCharacter
{
public:
	AlienCharacter(int type, int location);
	~AlienCharacter();

	void Update();

	void SaveAnimations();
	void LoadModel();
	void SetSpawnPosition();
	void SetSpawnAngle();
	void SetupShaders();

private:
	// 적 정보
	glm::vec3 alienPos;
	float viewingAngle;
	int state = { 0 };     // 0: Idle, 1: Run, 2:Attack, 3: Hit, 4: Die, 5: Dance

	// 적 종류와 위치
	int alienType;
	int alienLocationSetter;

	// 애니메이션
	vector<BoneInfo>* alien_BoneInfo;
	AnimatedModel* animModel;
	AnimInfo* alien_CurrentAnim;
	AnimatedModel::AnimationLibrary* animLibrary = { nullptr };
	std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;

	// 캐릭터 OPENGL
	GLuint aVAO, aVBO, aVBO2, aEBO, aTexture, aShaderprogram;
	std::vector<unsigned int> aIndices;

	// 공격선 OPENGL
	GLuint lVAO, lVBO, lShaderprogram;
};

