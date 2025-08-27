#pragma once

class Fade
{
public:
	void Init();
	void Update();
	void Render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos, const glm::vec3& frontDir);
	void Release();

	float GetFadeAlpha() const { return fadeAlpha; }
	void AddFadeAlpha(const float deltaTime) { fadeAlpha += 0.25f * deltaTime; }
	void SubtractFadeAlpha(const float deltaTime) { fadeAlpha -= 0.25f * deltaTime; }

private:
	void CreateQuad();

private:
	GLuint VAO, VBO, EBO, ShaderProgram;
	GLuint mvpLocation, colorLocation;

	float fadeAlpha = { 0.0f };
};

