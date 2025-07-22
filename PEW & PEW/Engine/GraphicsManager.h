#pragma once

class Camera;
class ShadowMapping;
class NetworkManager;
class MainCharacter;
class AlienCharacter;

class GraphicsManager
{
public:
	void Init();
	void Update();
	void Render(GLFWwindow* window);
	void RenderShadow();
	void Release();

	void InitAlienCharacters();
	void UpdateAlienCharacters(float deltatime);

	void AddCharacter(int id, bool isLocal = false);
	void RemoveCharacter(int id);
	MainCharacter* GetCharacter(int id);
	MainCharacter* GetLocalCharacter();

	Camera* GetCamera() const;
	MainCharacter* GetMainCat();
	void SetNetworkManager(NetworkManager* net);
	void DebugAllCharacterPositions();

	const std::map<int, MainCharacter*>& GetAllCharacters() const { return catCharacters; }

private:
	Camera* camera = { nullptr };
	ShadowMapping* shadowMap = { nullptr };
	NetworkManager* network = { nullptr };
	std::map<int, MainCharacter*> catCharacters;  // 모든 캐릭터 (ID 기반)
	int myPlayerID = { -1 };
	std::array<std::array<AlienCharacter*, 9>, 3> alienCharacters;
};
