#pragma once
#include "SceneManager.h"

class Camera;
class ShadowMapping;
class NetworkManager;
class MainCharacter;
class AlienCharacter;
class SceneManager;

class GraphicsManager
{
public:
	void Init();
	void Update(SceneType type);
	void Render(GLFWwindow* window, SceneType type);
	void RenderShadow(SceneType type);
	void Release();

	void InitAlienCharacters();
	void UpdateAlienCharacters(float deltatime);

	void AddCharacter(int id, bool isLocal = false, float speed = 0.1f);
	void RemoveCharacter(int id);
	MainCharacter* GetCharacter(int id);
	MainCharacter* GetLocalCharacter();

	Camera* GetCamera() const;
	MainCharacter* GetMainCat();
	void DebugAllCharacterPositions();

	const std::map<int, MainCharacter*>& GetAllCharacters() const { return catCharacters; }

	void SetSceneManager(SceneManager* sm);

private:
	Camera* camera = { nullptr };
	ShadowMapping* shadowMap = { nullptr };
	std::map<int, MainCharacter*> catCharacters;  // 모든 캐릭터 (ID 기반)
	int myPlayerID = { -1 };
	std::array<std::array<AlienCharacter*, 9>, 3> alienCharacters;
};
