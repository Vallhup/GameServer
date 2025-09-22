#include "pch.h"
#include "Bullet.h"
#include "stb_image.h"
#include "MainCharacter.h"
#include "Camera.h"
#include "Timer.h"
#include "AlienCharacter.h"

Bullet::Bullet(int type, float speed)
{
	SetupShader("Shaders/StaticObjectVert.glsl", "Shaders/StaticObjectFrag.glsl", shaderprogram);
	bulletType = type;
	SelectBulletType(speed);
}

Bullet::~Bullet()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderprogram);
}

void Bullet::SelectBulletType(float speed)
{
	if (bulletType == 1)
	{
		LoadBulletGLB("StaticGlb/dagger.glb");
		Texture = LoadBulletTexture("Texture/dagger.png");
		bulletSpeed = speed;
	}
	else if (bulletType == 2)
	{
		LoadBulletGLB("StaticGlb/star.glb");
		Texture = LoadBulletTexture("Texture/star.png");
		bulletSpeed = speed;
	}
}

void Bullet::LoadBulletGLB(const std::string& filename) {
	const aiScene* scene = objectImporter.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace);
	if (!scene) {
		std::cout << "Failed to load GLB file: " << objectImporter.GetErrorString() << std::endl;
		return;
	}

	std::vector<float> vertexData;
	Indices.clear();

	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
			vertexData.push_back(mesh->mVertices[j].x);
			vertexData.push_back(mesh->mVertices[j].y);
			vertexData.push_back(mesh->mVertices[j].z);

			if (mesh->HasNormals()) {
				vertexData.push_back(mesh->mNormals[j].x);
				vertexData.push_back(mesh->mNormals[j].y);
				vertexData.push_back(mesh->mNormals[j].z);
			}
			else {
				vertexData.push_back(0.0f);
				vertexData.push_back(1.0f);
				vertexData.push_back(0.0f);
			}

			if (mesh->HasTextureCoords(0)) {
				vertexData.push_back(mesh->mTextureCoords[0][j].x);
				vertexData.push_back(mesh->mTextureCoords[0][j].y);
			}
			else {
				vertexData.push_back(0.0f);
				vertexData.push_back(0.0f);
			}
		}

		for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			for (unsigned int k = 0; k < face.mNumIndices; k++)
				Indices.push_back(face.mIndices[k]);
		}
	}

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(unsigned int), Indices.data(), GL_STATIC_DRAW);

	const GLsizei stride = (3 + 3 + 2) * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//cout << "File loaded: " << filename << '\n';
}

GLuint Bullet::LoadBulletTexture(const char* path)
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

void Bullet::BulletSetting(MainCharacter* character, Camera* camera, glm::vec3 mousePick)
{
	position = character->GetPosition();
	position.y = 0.45f;

	float angle = atan2(mouseDir.x, mouseDir.z);

	if (!camera->GetViewType()) {

		position.x += cos(angle) * 0.2f;
		position.z -= sin(angle) * 0.2f;

		glm::vec3 targetPos = mousePick;
		targetPos.y = 0.45f;
		direction = glm::normalize(targetPos - position);
	}
	else {
		float horizontalAngle = camera->GetHorizontalAngle();
		float verticalAngle = camera->GetVerticalAngle();

		direction = glm::vec3(
			sin(horizontalAngle) * cos(verticalAngle),
			-sin(verticalAngle),
			cos(horizontalAngle) * cos(verticalAngle)
		);
	}

	model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	
}

void Bullet::Render(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
	glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
	glUseProgram(shaderprogram);

	// TODO: 캐싱 처리하자.
	ViewLoc = glGetUniformLocation(shaderprogram, "view");
	ProjLoc = glGetUniformLocation(shaderprogram, "projection");
	ModelLoc = glGetUniformLocation(shaderprogram, "model");

	glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &orgview[0][0]);
	glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &orgproj[0][0]);
	glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "lightSpaceMatrix"),
		1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glUniform1i(glGetUniformLocation(shaderprogram, "shadowMap"), 1);

	GLuint lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
	GLuint viewPosLoc = glGetUniformLocation(shaderprogram, "viewPos");
	glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
	glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glUniform1i(glGetUniformLocation(shaderprogram, "objTexture"), 0);
	glUniform1i(glGetUniformLocation(shaderprogram, "useobjTexture"), 1);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
}

void Bullet::RenderShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
	glUseProgram(depthShader);
	glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"),
		1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"),
		1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
}

void Bullet::CatBulletUpdateFromServer(float deltaTime)
{
	glm::vec3 dir = targetPos - position;
	float dist = glm::length(dir);

	if (dist > 0.001f) {
		float moveDist = bulletSpeed * deltaTime;
		float alpha = glm::clamp(moveDist / dist, 0.0f, 1.0f);
		position = glm::mix(position, targetPos, alpha);
	}

	model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
}

void Bullet::BulletUpdate(const float deltaTime, const float bulletspeed)
{
	position += direction * bulletspeed * deltaTime;

	model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	if (bulletType == 1)
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
	else if (bulletType == 2)
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
}

void Bullet::BulletSetting(const glm::vec3 alienPos, const glm::vec3 catPos)
{
	position = alienPos;
	position.y = 0.45f;

	targetPos = catPos;
	targetPos.y = 0.45f;
	direction = glm::normalize(targetPos - position);
	position += 0.8f * direction;
}

void Bullet::SetPosition(glm::vec3 startPos)
{
	targetPos = startPos;

	if (glm::length(position - targetPos) > 2.0f)
		position = targetPos;
}

bool Bullet::IsCollapsed(AlienCharacter* alien)
{
	bool check{ false };

	/*for (int i = 0; i < 70; ++i)
	{
		if (min_Z[i] <= position.z && max_Z[i] >= position.z)
		{
			if (position.x <= max_X[i] && position.x > min_X[i])
				check = true;
		}
	}*/

	glm::vec3 pos = alien->GetPosition();
	if (position.y >= 0.0f && position.y <= 0.95f)
	{
		if ((position.x >= pos.x - 0.25f && position.x <= pos.x + 0.25f) &&
			(position.z >= pos.z - 0.2f && position.z <= pos.z + 0.2f))
		{
			return true;
		}
	}

	return check;
}

bool Bullet::IsCollapsed(MainCharacter* Cat)
{
	bool check{ false };

	/*for (int i = 0; i < 70; ++i)
	{
		if (min_Z[i] <= position.z && max_Z[i] >= position.z)
		{
			if (position.x <= max_X[i] && position.x > min_X[i])
				check = true;
		}
	}*/

	glm::vec3 pos = Cat->GetPosition();

	if (!Cat->GetDead())
	{
		if (position.y >= 0.0f && position.y <= 0.95f)
		{
			if ((position.x >= pos.x - 0.25f && position.x <= pos.x + 0.25f) &&
				(position.z >= pos.z - 0.2f && position.z <= pos.z + 0.2f))
			{
				return true;
			}
		}
	}
	
	return check;
}
