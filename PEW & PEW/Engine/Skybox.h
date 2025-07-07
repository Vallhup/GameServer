#pragma once

class Skybox
{
	DECLARE_SINGLE(Skybox);

public:
	void Init();
	void Release();
	unsigned int LoadCubemap(vector<string> faces);
	void SetUpSkyboxVertices();

	void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
	GLuint VAO, VBO, shaderprogram;
	GLuint ViewLoc, ProjLoc;

private:
	vector<string> faces;
	float skyboxVertices[108];
	unsigned int cubemapTexture;
};