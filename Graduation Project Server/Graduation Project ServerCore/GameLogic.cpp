#include "pch.h"
#include "GameLogic.h"

GameLogic::GameLogic(Instance* instance) : _instance(instance)
{
}

void GameLogic::LogicUpdate(float deltaTime)
{
	for (auto& obj : _instance->GetGameObjectList()) {
		obj->LogicUpdate(deltaTime);
	}
}

void GameLogic::NetworkUpdate()
{
	for (auto& obj : _instance->GetGameObjectList()) {
		obj->NetworkUpdate();
	}
}

void GameLogic::OnPlayerAction(int sessionId, Protocol::CS_INPUT_PACKET& packet)
{
	_instance->GetGameObject(sessionId)->GetComponent<InputComponent>()->Enqueue(packet);
}