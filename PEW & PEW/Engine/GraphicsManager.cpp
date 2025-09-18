#include "pch.h"
#include "GraphicsManager.h"
#include "Skybox.h"
#include "StaticObjectManager.h"
#include "ShadowMapping.h"
#include "SoundManager.h"
#include "Camera.h"
#include "Timer.h"
#include "NetworkManager.h"
#include "MainCharacter.h"
#include "AlienCharacter.h"
#include "SceneManager.h"
#include "Fade.h"
#include "EffectManager.h"

void GraphicsManager::Init()
{
	GET_SINGLE(Skybox)->Init();
	GET_SINGLE(StaticObjectManager)->Init();

	camera = new Camera();
	shadowMap = new ShadowMapping();
	fade = new Fade();
	fade->Init();

	effect = new EffectManager();
	effect->Init();

	effect->PlayEffect("CandleFire", glm::vec3{ -35.0322f, 2.0f, 44.6548f });

	glm::vec3 localPos = glm::vec3(-37.3051f, 0.0f, 42.5001f);
	AddCharacter(0, localPos, 0, true, 0.1f);
	InitAlienCharacters();
}

void GraphicsManager::InitPVPMap()
{
	GET_SINGLE(StaticObjectManager)->InitPVPMap();
}

void GraphicsManager::Update(SceneType type, SoundManager& soundmanager, const float deltaTime)
{
	camera->Update(soundmanager, deltaTime);

	int scenetype = static_cast<int>(type);

	if (type == SceneType::Scene1)
	{
		MainCharacter* cat = GetLocalCharacter();
		cat->Update(deltaTime, alienCharacters);
		UpdateAlienCharacters(deltaTime);
		GET_SINGLE(StaticObjectManager)->Update(deltaTime);
	}
	else
	{
		for (auto& [id, character] : catCharacters) {
			character->Update(deltaTime);
		}

		MainCharacter* localPlayer = GetLocalCharacter();
		if (localPlayer) {
			GET_SINGLE(StaticObjectManager)->UpdatePVPPlayerPosition(localPlayer->GetPosition());
		}

		GET_SINGLE(StaticObjectManager)->Update(deltaTime);
	}

	effect->Update(deltaTime);

	UpdateLightAngle(deltaTime);
}

void GraphicsManager::UpdateLightAngle(const float deltaTime)
{
	if (light_angle > 6.28f)
		light_angle -= 6.28f;
	light_angle += 0.1f * deltaTime;
}

void GraphicsManager::Render(SceneType type, SoundManager& soundmanager)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	float deltatime = GET_SINGLE(Timer)->GetDeltaTime();

	MainCharacter* localChar = GetLocalCharacter();
	if (!localChar) return;  // 로컬 캐릭터가 없으면 렌더링 하지 않음

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
	glm::mat4 view = camera->GetViewMatrix(localChar->GetPosition());
	glm::vec3 viewPos = camera->GetPosition(localChar->GetPosition());
	glm::mat4 lightSpaceMatrix = shadowMap->GetLightSpaceMatrix();

	RenderShadow(type);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Normal Pass 시작
	GET_SINGLE(Skybox)->Draw(view, projection);

	GET_SINGLE(StaticObjectManager)->Draw(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());

	if (type == SceneType::Scene1)
	{
		localChar->Draw(view, projection, viewPos, deltatime, lightSpaceMatrix, shadowMap->GetDepthMap());
		localChar->RenderBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());

		for (int type = 0; type < 3; ++type) {
			for (int location = 0; location < 9; ++location) {
				if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
					alienCharacters[type][location]->Draw(view, projection, viewPos, deltatime, lightSpaceMatrix, shadowMap->GetDepthMap());
					alienCharacters[type][location]->DrawBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());
				}
			}
		}
	}
	else
	{
		for (auto& [id, character] : catCharacters) {
			character->Draw(view, projection, viewPos, deltatime, lightSpaceMatrix, shadowMap->GetDepthMap());
			character->RenderBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());
		}
	}

	camera->Render();

	glm::vec3 cameraFront = camera->GetFrontVector(localChar->GetPosition());
	glm::vec3 cameraTarget = viewPos + cameraFront;
	effect->Render(viewPos, cameraTarget);

	RenderFade(projection, view, viewPos);

	if (!firstRenderDone)
	{
		soundmanager.PlayBGM();
		firstRenderDone = true;
	}

	glFinish();
}

void GraphicsManager::RenderFade(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& viewPos)
{
	MainCharacter* localChar = GetLocalCharacter();

	if (localChar) {
		glm::vec3 frontDir = camera->GetFrontVector(localChar->GetPosition());
		fade->Render(projection, view, viewPos, frontDir);
	}
	
}

void GraphicsManager::RenderShadow(SceneType type)
{
	MainCharacter* localChar = GetLocalCharacter();
	if (!localChar) return;

	shadowMap->UpdateLightSpaceMatrix(localChar->GetPosition());		// Shadow Pass 시작
	shadowMap->BindFramebuffer();
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 lightSpaceMatrix = shadowMap->GetLightSpaceMatrix();

	if (type == SceneType::Scene1)
	{
		localChar->DrawShadow(lightSpaceMatrix, shadowMap->GetDepthShaderProgram());

		for (int type = 0; type < 3; ++type) {
			for (int location = 0; location < 9; ++location) {
				if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
					alienCharacters[type][location]->DrawShadow(shadowMap);
				}
			}
		}
	}
	else
	{
		for (auto& [id, character] : catCharacters) {
			character->DrawShadow(lightSpaceMatrix, shadowMap->GetDepthShaderProgram());
		}
	}

	GET_SINGLE(StaticObjectManager)->DrawShadow(lightSpaceMatrix, shadowMap->GetStaticDepthShaderProgram());

	if (type == SceneType::Scene1)
	{
		localChar->RenderBulletsShadow(shadowMap->GetLightSpaceMatrix(), shadowMap->GetStaticDepthShaderProgram());

		for (int type = 0; type < 3; ++type) {
			for (int location = 0; location < 9; ++location) {
				if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
					alienCharacters[type][location]->DrawBulletsShadow(shadowMap->GetLightSpaceMatrix(), shadowMap->GetStaticDepthShaderProgram());
				}
			}
		}
	}
	else
	{
		for (auto& [id, character] : catCharacters) {
			character->RenderBulletsShadow(shadowMap->GetLightSpaceMatrix(), shadowMap->GetStaticDepthShaderProgram());
		}
	}

	shadowMap->UnbindFramebuffer();
	glViewport(0, 0, WIN_W, WIN_H);			// Shadow Pass 종료
}

void GraphicsManager::Release()
{
	GET_SINGLE(Skybox)->Release();
	GET_SINGLE(StaticObjectManager)->Release();

	// 모든 캐릭터 삭제
	for (auto& [id, character] : catCharacters) {
		delete character;
	}
	catCharacters.clear();

	for (int type = 0; type < 3; ++type) {
		for (int location = 0; location < 9; ++location) {
			if (alienCharacters[type][location]) {
				delete alienCharacters[type][location];
				alienCharacters[type][location] = nullptr;
			}
		}
	}

	delete shadowMap;
	delete camera;

	fade->Release();
	delete fade;

	effect->Release();
	delete effect;
}

void GraphicsManager::ReleaseScene1()
{
	GET_SINGLE(StaticObjectManager)->Release();
}

void GraphicsManager::InitAlienCharacters()
{
	// 3가지 타입 × 9개 위치 = 27마리 적 생성
	for (int type = 0; type < 3; ++type) {
		for (int location = 0; location < 9; ++location) {
			alienCharacters[type][location] = new AlienCharacter(type, location);
		}
	}
}

void GraphicsManager::UpdateAlienCharacters(float deltatime)
{
	MainCharacter* localChar = GetLocalCharacter();
	if (!localChar) return;  // 로컬 플레이어가 없으면 업데이트 안함

	for (int type = 0; type < 3; ++type) {
		for (int location = 0; location < 9; ++location) {
			if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
				alienCharacters[type][location]->Update(deltatime, localChar, alienCharacters);
			}
		}
	}
}

void GraphicsManager::AddCharacter(int id, glm::vec3 cPos, int characterType, bool isLocal, float speed)
{
	if (catCharacters.find(id) == catCharacters.end()) {
		MainCharacter* newChar = new MainCharacter(id, cPos, isLocal, speed);
		newChar->Init(characterType);
		catCharacters[id] = newChar;

		if (isLocal) {
			myPlayerID = id;
			newChar->SetCamera(camera);
		}

		std::cout << "[ADD CHARACTER] ID: " << id << (isLocal ? " (Local)" : " (Remote)") << std::endl;
	}
}

void GraphicsManager::RemoveCharacter(int id)
{
	auto it = catCharacters.find(id);
	if (it != catCharacters.end()) {
		if (it->first == myPlayerID) {
			myPlayerID = -1;  // 내 캐릭터가 삭제되면 ID 초기화
		}
		delete it->second;
		catCharacters.erase(it);
		std::cout << "[REMOVE CHARACTER] ID: " << id << std::endl;
	}
}

MainCharacter* GraphicsManager::GetCharacter(int id)
{
	auto it = catCharacters.find(id);
	if (it != catCharacters.end()) {
		return it->second;
	}
	return nullptr;
}

MainCharacter* GraphicsManager::GetLocalCharacter()
{
	if (myPlayerID != -1) {
		return GetCharacter(myPlayerID);
	}
	return nullptr;
}

Camera* GraphicsManager::GetCamera() const
{
	return camera;
}

MainCharacter* GraphicsManager::GetMainCat()
{
	return GetLocalCharacter();
}

Fade* GraphicsManager::GetFade()
{
	return fade;
}

void GraphicsManager::DebugAllCharacterPositions()
{
	std::cout << "\n=== 모든 캐릭터 위치 디버깅 ===" << std::endl;
	std::cout << "현재 접속 캐릭터 수: " << catCharacters.size() << std::endl;
	std::cout << "내 플레이어 ID: " << myPlayerID << std::endl;
	std::cout << "=============================" << std::endl;

	for (auto& pair : catCharacters) {
		int id = pair.first;
		MainCharacter* character = pair.second;

		if (character) {
			glm::vec3 pos = character->GetPosition();
			bool isLocal = character->CheckLocal();

			std::cout << "[ID: " << id << "] "
				<< (isLocal ? "(로컬)" : "(원격)")
				<< " 위치: ("
				<< pos.x << ", "
				<< pos.y << ", "
				<< pos.z << ")" << std::endl;

			// 원격 플레이어라면 추가 정보
			if (!isLocal) {
				std::cout << "    - 이동 상태: "
					<< (character->IsMoving() ? "이동중" : "정지") << std::endl;
				std::cout << "    - 달리기: "
					<< (character->GetShift() ? "ON" : "OFF") << std::endl;
			}
		}
		else {
			std::cout << "[ID: " << id << "] 캐릭터 객체가 nullptr!" << std::endl;
		}
	}
	std::cout << "=============================\n" << std::endl;
}

void GraphicsManager::SetSceneManager(SceneManager* sm) 
{
	MainCharacter* localChar = GetLocalCharacter();
	if (localChar) {
		localChar->SetSceneManager(sm);
	}
}
