#pragma once

class StaticObject;

class StaticObjectManager
{
	DECLARE_SINGLE(StaticObjectManager);

public:
	void Init();
	void Release();
	void Update();
	void Draw(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
		glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void DrawShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);

	StaticObject* AddStaticObject(const char* glb, const char* png);

private:
	vector<StaticObject*> StaticObjects;

};

