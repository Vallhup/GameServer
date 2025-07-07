#pragma once

class StaticObject
{
public:
	StaticObject(const char* glb, const char* png);
	~StaticObject();

	void LoadStaticObjectGLB(const std::string& filename);

	void drawStaticobject(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
		glm::mat4 lightSpaceMatrix, GLuint shadowMap);

	//void drawcloud(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	//void drawEnd(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void drawStaticobjectShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);
private:
	glm::vec3 position;

	GLuint VAO, VBO, EBO, shaderprogram, Texture;
	GLuint ViewLoc, ProjLoc, ModelLoc;

	std::vector<unsigned int> Indices;

	glm::mat4 model = { 1.0f };
	glm::mat4 projection = { 1.0f };
	glm::mat4 view = { 1.0f };

	Assimp::Importer objectImporter;
};