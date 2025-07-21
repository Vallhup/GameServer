#pragma once

class Character;
class Enemy;
class Camera;

class Bullet
{
public:
	Bullet();
	~Bullet();

	void LoadBulletGLB(const std::string& filename);
	GLuint LoadBulletTexture(const char* path);

	void BulletSetting(Character* character, Camera* camera, glm::vec3 mousePick);		// 나중에 1인칭 쓸거면 필요한 함수
	void Render(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void RenderShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);
	void BulletUpdate(float deltaTime);
	void SetPosition(glm::vec3 startPos);

private:
	glm::vec3 position;
	glm::vec3 targetPos;
	glm::vec3 direction{ 1.0f };
	float bulletSpeed{ 20.0f };

	GLuint VAO, VBO, EBO, shaderprogram, Texture;
	std::vector<unsigned int> Indices;
	GLuint ViewLoc, ProjLoc, ModelLoc;
	glm::mat4 model{ 1.0f };
	Assimp::Importer objectImporter;
};