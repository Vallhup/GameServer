#pragma once

class StaticObject;

class StaticObjectManager
{
	DECLARE_SINGLE(StaticObjectManager);

public:
	void Init();
	void InitPVPMap();
	void Release();
	void Update(const float deltaTime);
	void Draw(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
		glm::mat4 lightSpaceMatrix, GLuint shadowMap);
	void DrawShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader);

	StaticObject* AddStaticObject(const char* glb, const char* png, const char* let);

	void UpdatePVPPlayerPosition(const glm::vec3& pos);

	void SetPlayerState(PlayerPVPState state);
	PlayerPVPState GetPlayerState() const { return currentPlayerState; }
	bool ShouldRenderStateText(const std::string& textName) const;
	bool GetEndingState() const;

private:
	vector<StaticObject*> StaticObjects;
	float cloudPosition = 0.0f;

	PlayerPVPState currentPlayerState = PlayerPVPState::WAITING;
	float fightTextTimer = 0.0f;
	const float FIGHT_TEXT_DURATION = 1.5f;
	glm::vec3 pvpPlayerPosition = glm::vec3(0.0f);
	float endTimer = 0.0f;
	const float END_DURATION = 5.0f;
	bool endingScene = false;
};

