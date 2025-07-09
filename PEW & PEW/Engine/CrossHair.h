#pragma once

class CrossHair
{
public:
	CrossHair(int Viewtype, float size, GLFWwindow* win);
	~CrossHair();

	void SetupCrosshairShaders(const char* vertexName, const char* fragmentName);

	void RenderCrosshair();
private:
	GLuint VAO, VBO, EBO, ShaderProgram;
	int m_Viewtype;
	GLFWcursor* customCursor;
	GLFWwindow* window;
};

