#include "pch.h"
#include "AlienCharacter.h"

AlienCharacter::AlienCharacter(int type, int location)
{
	alien_BoneInfo = new vector<BoneInfo>();
	animModel = new AnimatedModel();
	alien_CurrentAnim = new AnimInfo();
	animLibrary = new AnimatedModel::AnimationLibrary();

	alienType = type + 1;
	alienLocationSetter = location;

	SaveAnimations();
	LoadModel();
	SetSpawnPosition();
}

AlienCharacter::~AlienCharacter()
{
	delete alien_CurrentAnim;
	delete alien_BoneInfo;
	delete animModel;
	delete animLibrary;

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &VBO2);
	glDeleteBuffers(1, &EBO);
	glDeleteTextures(1, &Texture);
	glDeleteProgram(shaderprogram);
}

void AlienCharacter::SaveAnimations()
{
	animLibrary->LoadAnimation("Idle", "Animations/alien_animation_idle.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Run", "Animations/alien_animation_run.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Attack", "Animations/alien_animation_attack.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Hit", "Animations/alien_animation_hit.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Death", "Animations/alien_animation_death.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Dance", "Animations/cat_animation_dance.glb", animationImporters, animModel);

	animLibrary->ChangeAnimation("Idle", *alien_CurrentAnim);
}

void AlienCharacter::LoadModel()
{
	if (alienType == 1)
	{
		animModel->LoadGLBFile(alienType, *alien_BoneInfo, "Glb/alien_1_Tpose.glb", VAO, VBO, VBO2, EBO, indices);
		Texture = LoadTexture("Texture/alien_1_basecolor.png");
	}
	else if (alienType == 2)
	{
		animModel->LoadGLBFile(alienType, *alien_BoneInfo, "Glb/alien_2_Tpose.glb", VAO, VBO, VBO2, EBO, indices);
		Texture = LoadTexture("Texture/alien_2_basecolor.png");
	}
	else if (alienType == 3)
	{
		animModel->LoadGLBFile(alienType, *alien_BoneInfo, "Glb/alien_3_Tpose.glb", VAO, VBO, VBO2, EBO, indices);
		Texture = LoadTexture("Texture/alien_3_basecolor.png");
	}
}

void AlienCharacter::SetSpawnPosition()
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

	alienPos = ENEMY_SPAWN_POINTS[(9 * (alienType - 1)) + alienLocationSetter];
}