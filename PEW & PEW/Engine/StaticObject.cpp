#include "pch.h"
#include "StaticObject.h"
#include "stb_image.h"

StaticObject::StaticObject(const char* glb, const char* png)
{
	SetupShader("Shaders/StaticObjectVert.glsl", "Shaders/StaticObjectFrag.glsl", shaderprogram);
	LoadStaticObjectGLB(glb);
	Texture = LoadTexture(png);
}

StaticObject::~StaticObject()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderprogram);
}


void StaticObject::LoadStaticObjectGLB(const std::string& filename) {
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
}

void StaticObject::drawStaticobject(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
	glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
	glUseProgram(shaderprogram);
	ViewLoc = glGetUniformLocation(shaderprogram, "view");
	glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &orgview[0][0]);
	ProjLoc = glGetUniformLocation(shaderprogram, "projection");
	glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &orgproj[0][0]);
	ModelLoc = glGetUniformLocation(shaderprogram, "model");
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

//void StaticObject::drawcloud(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
//	glm::mat4 lightSpaceMatrix, GLuint shadowMap)
//{
//	glUseProgram(shaderprogram);
//	ViewLoc = glGetUniformLocation(shaderprogram, "view");
//	glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &orgview[0][0]);
//	ProjLoc = glGetUniformLocation(shaderprogram, "projection");
//	glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &orgproj[0][0]);
//	if (cloud_go)
//	{
//		if (cloud_pos_z < 118.0f) {
//			model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.01f));
//			cloud_pos_z += 0.01f;
//		}
//		else
//		{
//			model = glm::translate(model, glm::vec3(0.0f, 0.0f, -236.0f));
//			cloud_pos_z -= 236.0f;
//		}
//	}
//	ModelLoc = glGetUniformLocation(shaderprogram, "model");
//	glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
//
//	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "lightSpaceMatrix"),
//		1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
//
//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, shadowMap);
//	glUniform1i(glGetUniformLocation(shaderprogram, "shadowMap"), 1);
//
//	GLuint lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
//	GLuint viewPosLoc = glGetUniformLocation(shaderprogram, "viewPos");
//	glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
//	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
//	glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, Texture);
//	glUniform1i(glGetUniformLocation(shaderprogram, "objTexture"), 0);
//	glUniform1i(glGetUniformLocation(shaderprogram, "useobjTexture"), 1);
//	glBindVertexArray(VAO);
//	glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
//}

//void StaticObject::drawEnd(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
//	glm::mat4 lightSpaceMatrix, GLuint shadowMap)
//{
//	glUseProgram(shaderprogram);
//	ViewLoc = glGetUniformLocation(shaderprogram, "view");
//	glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &orgview[0][0]);
//	ProjLoc = glGetUniformLocation(shaderprogram, "projection");
//	glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &orgproj[0][0]);
//	if (!send_end)
//	{
//		model = glm::translate(model, glm::vec3(0.0f, end_y, 0.0f));
//		send_end = true;
//	}
//	if (finish && (end_y < 0.0f))
//	{
//		model = glm::translate(model, glm::vec3(0.0f, 0.005f, 0.0f));
//		end_y += 0.005f;
//	}
//	ModelLoc = glGetUniformLocation(shaderprogram, "model");
//	glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
//
//	glUniformMatrix4fv(glGetUniformLocation(shaderprogram, "lightSpaceMatrix"),
//		1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
//
//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, shadowMap);
//	glUniform1i(glGetUniformLocation(shaderprogram, "shadowMap"), 1);
//
//	GLuint lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
//	GLuint viewPosLoc = glGetUniformLocation(shaderprogram, "viewPos");
//	glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
//	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
//	glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, Texture);
//	glUniform1i(glGetUniformLocation(shaderprogram, "objTexture"), 0);
//	glUniform1i(glGetUniformLocation(shaderprogram, "useobjTexture"), 1);
//	glBindVertexArray(VAO);
//	glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
//}

void StaticObject::drawStaticobjectShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
	glUseProgram(depthShader);
	glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"),
		1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"),
		1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
}