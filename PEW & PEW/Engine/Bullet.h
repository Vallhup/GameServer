#pragma once

class MainCharacter;
class Enemy;
class Camera;

class Bullet
{
public:
	Bullet(int type);
	~Bullet();

	void SelectBulletType();
	void LoadBulletGLB(const std::string& filename);
	GLuint LoadBulletTexture(const char* path);

	void BulletSetting(MainCharacter* character, Camera* camera, glm::vec3 mousePick);		// 나중에 1인칭 쓸거면 필요한 함수
	void Render(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void RenderShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);
	void BulletUpdate(float deltaTime);		// 캐릭터용
	void SetPosition(glm::vec3 startPos);

private:
	glm::vec3 position;
	glm::vec3 targetPos;
	glm::vec3 direction;
	float bulletSpeed = { 20.0f };
	int bulletType = { 0 };

	GLuint VAO, VBO, EBO, shaderprogram, Texture;
	std::vector<unsigned int> Indices;
	GLuint ViewLoc, ProjLoc, ModelLoc;
	glm::mat4 model = glm::mat4(1.0f);;
	Assimp::Importer objectImporter;
};