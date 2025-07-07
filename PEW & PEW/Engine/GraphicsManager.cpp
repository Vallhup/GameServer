#include "pch.h"
#include "GraphicsManager.h"
#include "Skybox.h"
#include "StaticObjectManager.h"
#include "MainCharacter.h"
#include "ShadowMapping.h"
#include "Camera.h"
#include "Enemy.h"
#include "Timer.h"

void GraphicsManager::Init()
{
	GET_SINGLE(Skybox)->Init();
	GET_SINGLE(StaticObjectManager)->Init();

	camera = new Camera();
	mainCat = new MainCharacter();
	shadowMap = new ShadowMapping();

	mainCat->SetCamera(camera);

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 9; ++j)
		{
			enemy[i][j] = new Enemy(i + 1, j);
		}
	}
}

void GraphicsManager::Update()
{
	float deltatime = GET_SINGLE(Timer)->GetDeltaTime();

	camera->Update();
	mainCat->Update();
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 9; ++j)
		{
			enemy[i][j]->Update(deltatime, mainCat->GetPosition(), enemy[i][j], mainCat);
		}
	}
}

void GraphicsManager::Render(GLFWwindow* window)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glfwGetCursorPos(window, &cur_x, &cur_y);
	float deltatime = GET_SINGLE(Timer)->GetDeltaTime();

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
	glm::mat4 view = camera->GetViewMatrix(mainCat->GetPosition());
	if (!camera->IsAltPressed())
		mouseDir = camera->SetMouseWorldDirection(cur_x, cur_y, projection, view, mainCat->GetPosition());
	glm::vec3 viewPos = camera->GetPosition(mainCat->GetPosition());
	glm::mat4 lightSpaceMatrix = shadowMap->GetLightSpaceMatrix();
	float angle = camera->GetAngle();

	mainCat->ChangeCatAnimation(view, projection);

	RenderShadow();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Normal Pass 시작
	GET_SINGLE(Skybox)->Draw(view, projection);

	GET_SINGLE(StaticObjectManager)->Draw(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());

	mainCat->Draw(view, projection, viewPos, deltatime, angle);
	mainCat->ThrowBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 9; ++j)
		{
			enemy[i][j]->Draw(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());
			enemy[i][j]->ThrowBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());
		}
	}

	glFinish();
}

void GraphicsManager::RenderShadow()
{
	shadowMap->UpdateLightSpaceMatrix(mainCat->GetPosition());		// Shadow Pass 시작
	shadowMap->BindFramebuffer();
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(shadowMap->GetDepthShaderProgram());			// Depth map 렌더링
	glm::mat4 lightSpaceMatrix = shadowMap->GetLightSpaceMatrix();
	GLuint lightSpaceMatrixLoc = glGetUniformLocation(shadowMap->GetDepthShaderProgram(), "lightSpaceMatrix");
	glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

	float angle = camera->GetAngle();

	mainCat->DrawMainCatShadow(angle, shadowMap->GetDepthShaderProgram(), shadowMap->GetLightSpaceMatrix());

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 9; ++j)
		{
			enemy[i][j]->DrawEnemyShadow(shadowMap);
		}
	}

	GET_SINGLE(StaticObjectManager)->DrawShadow(lightSpaceMatrix, shadowMap->GetStaticDepthShaderProgram());

	mainCat->DrawCatBulletShadow(shadowMap->GetLightSpaceMatrix(), shadowMap->GetStaticDepthShaderProgram());

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 9; ++j) {
			enemy[i][j]->DrawEnemyBulletShadow(lightSpaceMatrix, shadowMap->GetStaticDepthShaderProgram());
		}
	}

	shadowMap->UnbindFramebuffer();
	glViewport(0, 0, WIN_W, WIN_H);			// Shadow Pass 종료
}

void GraphicsManager::Release()
{
	GET_SINGLE(Skybox)->Release();
	GET_SINGLE(StaticObjectManager)->Release();
	delete mainCat;
	delete shadowMap;
	delete camera;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 9; ++j)
			delete enemy[i][j];
	}
}

Camera* GraphicsManager::GetCamera() const
{
	return camera;
}

MainCharacter* GraphicsManager::GetMainCat() const
{
	return mainCat;
}
