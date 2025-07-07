#include "pch.h"
#include "stb_image.h"

double cur_x = 0.0f;
double cur_y = 0.0f;
glm::vec3 mouseDir;
float light_angle = { 0.0f };

string LoadFile(const string& filename) {
	ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: cannot open \"" << filename << "\"" << std::endl;
		return "";
	}

	// 파일 크기 구하기
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	// 버퍼 할당 및 파일 읽기
	std::string buffer;
	buffer.resize(size);
	file.read(&buffer[0], size);
	file.close();

	return buffer;
}

GLuint LoadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data) {
		GLenum format{};
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else {
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void SetupShader(const char* vertexName, const char* fragmentName, GLuint& shaderName)
{
	assert(vertexName != nullptr);
	assert(fragmentName != nullptr);

	string vertSource = LoadFile(vertexName);
	string fragSource = LoadFile(fragmentName);

	if (vertSource.empty() || fragSource.empty()) {
		cerr << "Failed to load shader files" << endl;
		return;
	}

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char* vertSourcePtr = vertSource.c_str();
	glShaderSource(vertexShader, 1, &vertSourcePtr, NULL);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragSourcePtr = fragSource.c_str();
	glShaderSource(fragmentShader, 1, &fragSourcePtr, NULL);
	glCompileShader(fragmentShader);

	shaderName = glCreateProgram();
	glAttachShader(shaderName, vertexShader);
	glAttachShader(shaderName, fragmentShader);
	glLinkProgram(shaderName);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}