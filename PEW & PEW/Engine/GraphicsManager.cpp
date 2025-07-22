#include "pch.h"
#include "GraphicsManager.h"
#include "Skybox.h"
#include "StaticObjectManager.h"
#include "ShadowMapping.h"
#include "Camera.h"
#include "Timer.h"
#include "NetworkManager.h"
#include "MainCharacter.h"
#include "AlienCharacter.h"

void GraphicsManager::Init()
{
	GET_SINGLE(Skybox)->Init();
	GET_SINGLE(StaticObjectManager)->Init();

	camera = new Camera();
	shadowMap = new ShadowMapping();

	InitAlienCharacters();
}

void GraphicsManager::Update()
{
	float deltaTime = GET_SINGLE(Timer)->GetDeltaTime();

	camera->Update();

	for (auto& [id, character] : catCharacters) {
		character->Update(deltaTime);
	}

	UpdateAlienCharacters(deltaTime);
}

void GraphicsManager::Render(GLFWwindow* window)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	float deltatime = GET_SINGLE(Timer)->GetDeltaTime();

	MainCharacter* localChar = GetLocalCharacter();
	if (!localChar) return;  // 로컬 캐릭터가 없으면 렌더링 하지 않음

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
	glm::mat4 view = camera->GetViewMatrix(localChar->GetPosition());
	glm::vec3 viewPos = camera->GetPosition(localChar->GetPosition());
	glm::mat4 lightSpaceMatrix = shadowMap->GetLightSpaceMatrix();

	RenderShadow();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Normal Pass 시작
	GET_SINGLE(Skybox)->Draw(view, projection);

	GET_SINGLE(StaticObjectManager)->Draw(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());

	// 모든 캐릭터 렌더링
	for (auto& [id, character] : catCharacters) {
		character->Draw(view, projection, viewPos, deltatime, lightSpaceMatrix, shadowMap->GetDepthMap());
	}

	// 모든 캐릭터의 총알 렌더링 (로컬/원격 구분 없이)
	for (auto& [id, character] : catCharacters) {
		character->RenderBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());
	}

	for (int type = 0; type < 3; ++type) {
		for (int location = 0; location < 9; ++location) {
			if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
				alienCharacters[type][location]->Draw(view, projection, viewPos, deltatime, lightSpaceMatrix, shadowMap->GetDepthMap());
				alienCharacters[type][location]->DrawBullets(view, projection, viewPos, lightSpaceMatrix, shadowMap->GetDepthMap());
			}
		}
	}

	camera->Render();

	glFinish();
}

void GraphicsManager::RenderShadow()
{
	MainCharacter* localChar = GetLocalCharacter();
	if (!localChar) return;

	shadowMap->UpdateLightSpaceMatrix(localChar->GetPosition());		// Shadow Pass 시작
	shadowMap->BindFramebuffer();
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 lightSpaceMatrix = shadowMap->GetLightSpaceMatrix();

	for (auto& [id, character] : catCharacters) {
		character->DrawShadow(lightSpaceMatrix, shadowMap->GetDepthShaderProgram());
	}

	for (int type = 0; type < 3; ++type) {
		for (int location = 0; location < 9; ++location) {
			if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
				alienCharacters[type][location]->DrawShadow(shadowMap);
			}
		}
	}

	GET_SINGLE(StaticObjectManager)->DrawShadow(lightSpaceMatrix, shadowMap->GetStaticDepthShaderProgram());

	for (auto& [id, character] : catCharacters) {
		character->RenderBulletsShadow(shadowMap->GetLightSpaceMatrix(), shadowMap->GetStaticDepthShaderProgram());
	}

	for (int type = 0; type < 3; ++type) {
		for (int location = 0; location < 9; ++location) {
			if (alienCharacters[type][location] && !alienCharacters[type][location]->GetDead()) {
				alienCharacters[type][location]->DrawBulletsShadow(shadowMap->GetLightSpaceMatrix(), shadowMap->GetStaticDepthShaderProgram());
			}
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
				alienCharacters[type][location]->Update(deltatime, localChar);
			}
		}
	}
}

void GraphicsManager::AddCharacter(int id, bool isLocal)
{
	if (catCharacters.find(id) == catCharacters.end()) {
		MainCharacter* newChar = new MainCharacter(id, isLocal);
		newChar->Init();
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

void GraphicsManager::SetNetworkManager(NetworkManager* net)
{
	network = net;
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