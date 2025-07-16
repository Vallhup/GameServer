#pragma once

class Camera;
class ShadowMapping;
class Enemy;
class NetworkManager;
class Character;

class GraphicsManager
{
public:
	void Init();
	void Update();
	void Render(GLFWwindow* window);
	void RenderShadow();
	void Release();

	void SetMyPlayerID(int id);
	void AddCharacter(int id, bool isLocal = false);
	void RemoveCharacter(int id);
	Character* GetCharacter(int id);
	Character* GetLocalCharacter();

	Camera* GetCamera() const;
	Character* GetMainCat();
	void SetNetworkManager(NetworkManager* net);
	void DebugAllCharacterPositions();

	const std::map<int, Character*>& GetAllCharacters() const { return characters; }

private:
	Camera* camera = { nullptr };
	ShadowMapping* shadowMap = { nullptr };
	Enemy* enemy[3][9] = { nullptr };
	NetworkManager* network = { nullptr };
	std::map<int, Character*> characters;  // 모든 캐릭터 (ID 기반)
	int myPlayerID = -1;
};
