#pragma once
#include "AnimatedModel.h"

class AlienCharacter
{
public:
	AlienCharacter(int type, int location);
	~AlienCharacter();

	void SaveAnimations();
	void LoadModel();
	void SetSpawnPosition();

private:
	glm::vec3 alienPos;

	// 적 종류와 위치
	int alienType;
	int alienLocationSetter;

	// 애니메이션
	vector<BoneInfo>* alien_BoneInfo;
	AnimatedModel* animModel;
	AnimInfo* alien_CurrentAnim;
	AnimatedModel::AnimationLibrary* animLibrary = { nullptr };
	std::vector<std::unique_ptr<Assimp::Importer>> animationImporters;

	GLuint VAO, VBO, VBO2, EBO, Texture, shaderprogram;
	std::vector<unsigned int> indices;
};

