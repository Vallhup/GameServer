#pragma once
#include "SceneManager.h"

class Camera;
class ShadowMapping;
class NetworkManager;
class MainCharacter;
class AlienCharacter;
class SceneManager;
class Fade;

class GraphicsManager
{
public:
	void Init();
	void InitPVPMap();
	void Update(SceneType type, const float deltaTime);
	void Render(SceneType type);
	void RenderFade(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& viewPos);
	void RenderShadow(SceneType type);
	void Release();
	void ReleaseScene1();

	void InitAlienCharacters();
	void UpdateAlienCharacters(float deltatime);

	void AddCharacter(int id, glm::vec3 cPos, int characterType, bool isLocal = false, float speed = 0.1f);
	void RemoveCharacter(int id);
	MainCharacter* GetCharacter(int id);
	MainCharacter* GetLocalCharacter();

	Camera* GetCamera() const;
	MainCharacter* GetMainCat();
	Fade* GetFade();
	int GetCharacterType() const { return characterType; }
	void DebugAllCharacterPositions();

	const std::map<int, MainCharacter*>& GetAllCharacters() const { return catCharacters; }

	void SetSceneManager(SceneManager* sm);
	void SetCharacterType(int in) { characterType = in; }

private:
	int myPlayerID = { -1 };
	Camera* camera = { nullptr };
	ShadowMapping* shadowMap = { nullptr };
	Fade* fade = { nullptr };
	std::map<int, MainCharacter*> catCharacters;
	int characterType = { 0 };
	std::array<std::array<AlienCharacter*, 9>, 3> alienCharacters;
};
