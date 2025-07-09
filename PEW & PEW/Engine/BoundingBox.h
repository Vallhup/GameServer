#pragma once

class BoundingBox
{
public:
	BoundingBox();
	~BoundingBox();

	void RenderHitbox(float angle, const glm::vec3& pos, const glm::mat4& view, const glm::mat4& projection);

private:
	void SetupHitboxBuffers();
	void SetupHitboxShaders(const char* vertexName, const char* fragmentName);

private:
	GLuint VBO, VAO, Hitboxsh;
	glm::mat4 model{ 1.0f };
};

