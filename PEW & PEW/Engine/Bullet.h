#pragma once

class Character;
class Enemy;
class Camera;

class Bullet
{
public:
	Bullet(int type, int i, int j);
	~Bullet();

	void SelectBulletType(int type, int i, int j);

	void LoadBulletGLB(const std::string& filename);
	GLuint LoadBulletTexture(const char* path);

	void BulletSetting(Character* character, Camera* camera, glm::vec3 mousePick);
	void Render(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
		glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void RenderShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);

	void BulletUpdate(float deltaTime);

	glm::vec3 GettPos() { return tPos; }
	void SetPosition(glm::vec3 startPos);
	void SetDirection(glm::vec3 startPos, glm::vec3 endPos) { direction = glm::normalize(endPos - startPos); }

private:
	glm::vec3 position;
	glm::vec3 targetPos;
	float lerpSpeed = 1.0f;
	GLuint VAO, VBO, EBO, shaderprogram, Texture;
	std::vector<unsigned int> Indices;
	GLuint ViewLoc, ProjLoc, ModelLoc;
	glm::mat4 model{ 1.0f };
	Assimp::Importer objectImporter;
	bool shoot{ false };
	glm::vec3 direction{ 1.0f };
	float bulletSpeed{ 20.0f };
	int b_type{ 0 };
	glm::vec3 tPos;
	int enemy_i{}, enemy_j{};
};