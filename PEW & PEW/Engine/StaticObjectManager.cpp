#include "pch.h"
#include "StaticObjectManager.h"
#include "StaticObject.h"
#include "WindowInfo.h"

void StaticObjectManager::Init()
{
	AddStaticObject("StaticGlb/ground.glb", "Texture/map.png", "Ground");
	AddStaticObject("StaticGlb/title.glb", "Texture/title.png", "Title");
	AddStaticObject("StaticGlb/fence.glb", "Texture/fence.png", "Fence1");
	AddStaticObject("StaticGlb/tree1.glb", "Texture/tree1.png", "Tree1");
	AddStaticObject("StaticGlb/tree2.glb", "Texture/tree2.png", "Tree2");
	AddStaticObject("StaticGlb/tree3.glb", "Texture/tree3.png", "Tree3");
	AddStaticObject("StaticGlb/tree4.glb", "Texture/tree4.png", "Tree4");
	AddStaticObject("StaticGlb/bridge.glb", "Texture/bridge.png", "Bridge");
	AddStaticObject("StaticGlb/box.glb", "Texture/box.png", "Box");
	AddStaticObject("StaticGlb/cart.glb", "Texture/cart.png", "Cart");
	AddStaticObject("StaticGlb/housemain.glb", "Texture/housemain.png", "Housemain");
	AddStaticObject("StaticGlb/house1.glb", "Texture/house1.png", "House1");
	AddStaticObject("StaticGlb/house2.glb", "Texture/house2.png", "House2");
	AddStaticObject("StaticGlb/house3.glb", "Texture/house3.png", "House3");
	AddStaticObject("StaticGlb/house4.glb", "Texture/house4.png", "House4");
	AddStaticObject("StaticGlb/house5.glb", "Texture/house5.png", "House5");
	AddStaticObject("StaticGlb/house6.glb", "Texture/house6.png", "House6");
	AddStaticObject("StaticGlb/house7.glb", "Texture/house7.png", "House7");
	AddStaticObject("StaticGlb/house8.glb", "Texture/house8.png", "House8");
	AddStaticObject("StaticGlb/house9.glb", "Texture/house9.png", "House9");
	AddStaticObject("StaticGlb/rock1.glb", "Texture/rock1.png", "Rock1");
	AddStaticObject("StaticGlb/rock2.glb", "Texture/rock2.png", "Rock2");
	AddStaticObject("StaticGlb/waterwheel.glb", "Texture/waterwheel.png", "Waterwheel");
	AddStaticObject("StaticGlb/windmill.glb", "Texture/windmill.png", "Windmill");
	AddStaticObject("StaticGlb/cloud.glb", "Texture/cloud.png", "Cloud");
	AddStaticObject("StaticGlb/cave.glb", "Texture/cave.png", "Cave");
	AddStaticObject("StaticGlb/startlogo.glb", "Texture/startlogo.png", "Startlogo");
	AddStaticObject("StaticGlb/gameclear.glb", "Texture/gameclear.png", "GameClear");
}

void StaticObjectManager::InitPVPMap()
{
	AddStaticObject("StaticGlb/ground2.glb", "Texture/map2.png", "Map2");
	AddStaticObject("StaticGlb/fence4.glb", "Texture/fence4.png", "Fence2");
	AddStaticObject("StaticGlb/wordWaiting.glb", "Texture/wordWaiting.png", "Waiting");
	AddStaticObject("StaticGlb/wordReady.glb", "Texture/wordReady.png", "Ready");
	AddStaticObject("StaticGlb/wordFight.glb", "Texture/wordFight.png", "Fight");
	AddStaticObject("StaticGlb/wordWin.glb", "Texture/wordWin.png", "Win");
	AddStaticObject("StaticGlb/wordLose.glb", "Texture/wordLose.png", "Lose");
}

void StaticObjectManager::Release()
{
	for (auto& obj : StaticObjects)
	{
		delete obj;
	}
	StaticObjects.clear();
}

void StaticObjectManager::Update(const float deltaTime)
{
	for (auto& obj : StaticObjects)
	{
		if (obj->GetName() == "Cloud")
		{	
			cloudPosition += 1.0f * deltaTime;
			obj->MoveStaticobject(deltaTime);

			if (cloudPosition >= 118.0f)
			{
				obj->MoveStaticobjectToBeginPos();
				cloudPosition -= 236.0f;
			}
		}
	}

	if (currentPlayerState == PlayerPVPState::FIGHT && fightTextTimer > 0.0f) {
		fightTextTimer -= deltaTime;
	}

	if ((currentPlayerState == PlayerPVPState::WIN || currentPlayerState == PlayerPVPState::LOSE) && endTimer > 0.0f) {
		endTimer -= deltaTime;

		cout << "EndTimer: " << endTimer << endl;

		if (endTimer <= 0.0f)
		{
			if (!endingScene)
				endingScene = true;
		}
	}
}

void StaticObjectManager::Draw(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
	glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
	for (auto& obj : StaticObjects) {
		const std::string& objName = obj->GetName();

		if (objName == "Waiting" || objName == "Ready" || objName == "Fight" ||
			objName == "Win" || objName == "Lose") {

			if (ShouldRenderStateText(objName)) {
				obj->drawStaticobject(orgview, orgproj, viewPos, lightSpaceMatrix, shadowMap);
			}
		}
		else {
			obj->drawStaticobject(orgview, orgproj, viewPos, lightSpaceMatrix, shadowMap);
		}
	}
}

void StaticObjectManager::DrawShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
	for (auto& obj : StaticObjects) {
		const std::string& objName = obj->GetName();

		if (objName == "Waiting" || objName == "Ready" || objName == "Fight" ||
			objName == "Win" || objName == "Lose") {

			if (ShouldRenderStateText(objName)) {
				obj->drawStaticobjectShadow(lightSpaceMatrix, depthShader);
			}
		}
		else {
			obj->drawStaticobjectShadow(lightSpaceMatrix, depthShader);
		}
	}
}

StaticObject* StaticObjectManager::AddStaticObject(const char* glb, const char* png, const char* let)
{
	StaticObject* obj = new StaticObject(glb, png, let);
	StaticObjects.push_back(obj);
	return obj;
}

void StaticObjectManager::UpdatePVPPlayerPosition(const glm::vec3& pos)
{
	pvpPlayerPosition = pos;

	for (auto& obj : StaticObjects) {
		const std::string& objName = obj->GetName();

		if (objName == "Waiting" || objName == "Ready" || objName == "Fight" ||
			objName == "Win" || objName == "Lose") {

			glm::vec3 textPos = pos;
			textPos.y += 4.5f;
			textPos.z += 3.0f;  

			obj->SetPosition(textPos);

			if (objName == "Waiting")
			{
				glm::vec3 scale = glm::vec3(0.5f);
				obj->SetScale(scale);
			}
		}
	}
}

void StaticObjectManager::SetPlayerState(PlayerPVPState state)
{
	currentPlayerState = state;

	if (state == PlayerPVPState::FIGHT) {
		fightTextTimer = FIGHT_TEXT_DURATION; 
	}

	if (state == PlayerPVPState::WIN || state == PlayerPVPState::LOSE) {
		endTimer = END_DURATION;
	}
}

bool StaticObjectManager::ShouldRenderStateText(const std::string& textName) const
{
	switch (currentPlayerState) {

	case PlayerPVPState::WAITING:
		return textName == "Waiting";

	case PlayerPVPState::READY:
		return textName == "Ready";

	case PlayerPVPState::FIGHT:
		return textName == "Fight" && fightTextTimer > 0.0f;

	case PlayerPVPState::WIN:
		return textName == "Win";

	case PlayerPVPState::LOSE:
		return textName == "Lose";

	default:
		return false;
	}
}

bool StaticObjectManager::GetEndingState() const
{
	return endingScene;
}
